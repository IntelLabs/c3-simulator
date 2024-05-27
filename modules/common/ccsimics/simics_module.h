/*
 Copyright 2016 Intel Corporation
 SPDX-License-Identifier: MIT
*/
#ifndef MODULES_COMMON_CCSIMICS_SIMICS_MODULE_H_
#define MODULES_COMMON_CCSIMICS_SIMICS_MODULE_H_

#include <set>
#include <simics/simulator-iface/instrumentation-tool.h>
#include <simics/simulator/conf-object.h>
#include <simics/simulator/output.h>

// Must be defined in .cpp file instantiating the SimicsModule template!
extern conf_class_t *connection_class;

/**
 * @brief Abstract SimicsModule for registering module with Simics
 *
 * This should be overriden by the module, which should then define its own
 * Simics init_local that ivokes the SimicsModule init functions to register
 * the relvant types. When connecting the module to Simics, an object
 * of ConnectionTy is then created for each of the connections.
 *
 * A concrete model should define the following static constants as they will
 * be used during the registration of the module to Simics:
 *
 *  static constexpr const char *kModelName = "c3_model";
 *  static constexpr const char *kModelConnection = "c3_model_connection";
 *  static constexpr const char *kModelDescription = "C3";
 *
 * @tparam SimicsModuleTy Subclass type, needed for static casts from void *
 * @tparam ConnectionTy SimicsConnection type connecting to simulation
 */
template <typename SimicsModuleTy, typename ConnectionTy> class SimicsModule {
 private:
    conf_object_t *obj_;
    std::set<ConnectionTy *> connections_;
    unsigned next_connection_number_ = 0;

 public:
    inline explicit SimicsModule(conf_object_t *obj) : obj_(obj) {}

    inline ~SimicsModule() {
        ASSERT_FMT(connections_.empty(), "[%s] deleted with active connections",
                   SimicsModuleTy::kModelName);
    }

    /**
     * @brief Get the name object via SIM_object_name
     *
     * @return auto
     */
    inline auto get_name() const { return SIM_object_name(obj_); }

    /**
     * @brief Connect model to CPU using ConnectionTy
     *
     * Called automatically by registered Simics callbakcs.
     *
     * @param cpu
     * @param attr
     * @return conf_object*
     */
    inline conf_object *connect(conf_object_t *cpu, attr_value_t attr) {
        // Create the Simics connection_class
        strbuf_t sb = SB_INIT;
        sb_addfmt(&sb, "%s_%d", get_name(), next_connection_number_);
        conf_object_t *con_obj =
                SIM_create_object(connection_class, sb_str(&sb), attr);
        sb_free(&sb);

        // Get the contained ConnectionTy object
        auto *con = static_cast<ConnectionTy *>(SIM_object_data(con_obj));

        // Store the connection here
        ASSERT_MSG(next_connection_number_ != UINT_MAX, "Integer overflow!");
        ++next_connection_number_;
        connections_.insert(con);

        // Configure the connection
        con->module_configure(cpu, attr);
        con->configure();
        return con_obj;
    }

    /**
     * @brief Disconnect a conneciton from module
     *
     * Automatically called by Simics callbacks, doesn't need to be explicitly
     * called by sublcassing modules.
     *
     * @param con_obj
     */
    inline void disconnect(conf_object_t *con_obj) {
        auto *con = static_cast<ConnectionTy *>(SIM_object_data(con_obj));
        connections_.erase(con);
        SIM_delete_object(con_obj);
    }

    /**
     * @brief Print stats
     *
     * Override in subclass and call this via SimicsModule::print_stats() to
     * invoke print_stats for any connections to the simulation that may print
     * additional per-connection stats.
     *
     * Prints nothign by default.
     *
     */
    virtual inline void print_stats() {
        for (auto *c : connections_) {
            c->print_stats();
        }
    }

    /* static functions */

    static inline SimicsModuleTy *from_obj(conf_object_t *obj) {
        return static_cast<SimicsModuleTy *>(SIM_object_data(obj));
    }

    /**
     * @brief Used to regiser model with Simics during local_init
     *
     * @return conf_class*
     */
    static inline conf_class *init_model() {
        static const class_data_t kFuncs = {
                .alloc_object = NULL,
                .init_object =
                        [](conf_object_t *obj, lang_void *unused) {
                            SIM_printf("[%s] SimicsModule init_object.\n",
                                       SimicsModuleTy::kModelDescription);
                            return static_cast<lang_void *>(
                                    new SimicsModuleTy(obj));
                        },
                .pre_delete_instance = NULL,
                .delete_instance =
                        [](conf_object_t *obj) {
                            SIM_printf("[%s] SimicsModule delete.\n",
                                       SimicsModuleTy::kModelDescription);
                            delete SimicsModuleTy::from_obj(obj);
                            return 0;
                        },
                .description = SimicsModuleTy::kModelDescription,
                .kind = Sim_Class_Kind_Session};
        conf_class_t *cl =
                SIM_register_class(SimicsModuleTy::kModelName, &kFuncs);

        static const instrumentation_tool_interface_t kIt = {
                .connect =
                        [](auto *obj, auto *cpu, auto attr) {
                            auto *con_obj =
                                    SimicsModuleTy::from_obj(obj)->connect(
                                            cpu, attr);
                            SIM_printf("[%s] SimicsModule connect.\n",
                                       SimicsModuleTy::kModelDescription);
                            return con_obj;
                        },
                .disconnect =
                        [](auto *obj, auto *con_obj) {
                            SIM_printf("[%s] SimicsModule disconnect.\n",
                                       SimicsModuleTy::kModelDescription);
                            SimicsModuleTy::from_obj(obj)->disconnect(con_obj);
                        }};
        SIM_REGISTER_INTERFACE(cl, instrumentation_tool, &kIt);

        register_common_model_attributes(cl);
        SimicsModuleTy::register_model_specific_attributes(cl);
        return cl;
    }

    /**
     * @brief Used to regsiter connection with Simics during local_init
     *
     */
    static inline conf_class *init_connection() {
        static const class_data_t kIcFuncs = {
                .alloc_object = NULL,
                .init_object =
                        [](conf_object_t *con_obj, lang_void *unused) {
                            auto *con = new ConnectionTy(con_obj);
                            SIM_printf("[%s] SimicsConnection init_object.\n",
                                       SimicsModuleTy::kModelDescription);
#ifdef PAGING_5LVL
                            SIM_printf("[%s] Using 5-level paging.\n",
                                       MODEL_DESCRIPTION);
#endif
                            return static_cast<lang_void *>(con);
                        },
                .pre_delete_instance =
                        [](conf_object_t *obj) {
                            SIM_printf("[%s] SimicsConnection pre_delete.\n",
                                       SimicsModuleTy::kModelDescription);
                            auto *con = static_cast<ConnectionTy *>(
                                    SIM_object_data(obj));
                            con->ci_iface->remove_connection_callbacks(
                                    con->cpu_, obj);
                        },
                .delete_instance =
                        [](conf_object_t *obj) {
                            SIM_printf("[%s] SimicsConnection delete.\n",
                                       SimicsModuleTy::kModelDescription);
                            auto *con = static_cast<ConnectionTy *>(
                                    SIM_object_data(obj));
                            delete con;
                            return 0;
                        },
                .description = "Instrumentation connection",
                .kind = Sim_Class_Kind_Session};
        conf_class_t *cl =
                SIM_register_class(SimicsModuleTy::kModelConnection, &kIcFuncs);

        static const instrumentation_connection_interface_t kIcIface = {
                .enable =
                        [](auto *obj) {
                            auto *con = static_cast<ConnectionTy *>(
                                    SIM_object_data(obj));
                            con->ci_iface->enable_connection_callbacks(
                                    con->cpu_, obj);
                        },
                .disable =
                        [](auto *obj) {
                            auto *con = static_cast<ConnectionTy *>(
                                    SIM_object_data(obj));
                            con->ci_iface->disable_connection_callbacks(
                                    con->cpu_, obj);
                        }};
        SIM_REGISTER_INTERFACE(cl, instrumentation_connection, &kIcIface);
        connection_class = cl;
        register_common_connection_attributes(cl);
        ConnectionTy::register_common_connection_attributes(cl);
        ConnectionTy::register_connection_specific_attributes(cl);
        return cl;
    }

 private:
    /**
     * @brief Common connection attributes
     *
     * The ConnectionTy::register_connection_specific_attributes() can be
     * overriden in the ConnecitonTy to define custom attributes.
     *
     * @param cl The Simics connection class
     */
    static inline void register_common_connection_attributes(conf_class_t *cl) {
        SIM_register_typed_attribute(
                cl, "cpu",
                [](void *id, conf_object_t *obj, attr_value_t *idx) {
                    auto *con =
                            static_cast<ConnectionTy *>(SIM_object_data(obj));
                    return SIM_make_attr_object(con->cpu_);
                },
                NULL, NULL, NULL, Sim_Attr_Pseudo, "n|o", NULL,
                "to read out the cpu");
    }

    /**
     * @brief Common model attributes
     *
     * @param cl The Simics too/model class
     */
    static inline void register_common_model_attributes(conf_class_t *cl) {
        SIM_register_attribute(
                cl, "print_stats",
                [](conf_object_t *o) {
                    auto *m = static_cast<SimicsModuleTy *>(SIM_object_data(o));
                    m->print_stats();
                    return SIM_make_attr_int64(0);
                },
                NULL, Sim_Attr_Pseudo, "a", "Printing R/W stats");
    }
};

#endif  // MODULES_COMMON_CCSIMICS_SIMICS_MODULE_H_
