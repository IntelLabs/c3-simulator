// Copyright 2016-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef MODULES_COMMON_CCSIMICS_SIMICS_CONNECTION_H_
#define MODULES_COMMON_CCSIMICS_SIMICS_CONNECTION_H_

#include <simics/base/conf-object.h>
#include <simics/base/memory-transaction.h>
#include <simics/base/types.h>
#include <simics/model-iface/cpu-instrumentation.h>
#include <simics/model-iface/exception.h>
#include <simics/model-iface/int-register.h>
#include <simics/model-iface/processor-info.h>
#include <simics/processor/types.h>
#include <simics/simulator-api.h>
#include <simics/simulator-iface/instrumentation-tool.h>
#include <simics/simulator/conf-object.h>
#include <simics/simulator/output.h>
#include <simics/util/alloc.h>
#include <simics/util/hashtab.h>
#include "ccsimics/simics_util.h"

#define DECLARE(option, desc) bool option = false;

#define ATTR_REGISTER(option, desc)                                            \
    SIM_register_typed_attribute(                                              \
            connection_class, #option,                                         \
            [](void *arg, auto *obj, auto *idx) {                              \
                auto *con = static_cast<ConnectionTy *>(SIM_object_data(obj)); \
                return SIM_make_attr_boolean(con->option);                     \
            },                                                                 \
            NULL,                                                              \
            [](void *arg, auto *obj, auto *val, auto *idx) {                   \
                auto *con = static_cast<ConnectionTy *>(SIM_object_data(obj)); \
                con->option = SIM_attr_boolean(*val);                          \
                return Sim_Set_Ok;                                             \
            },                                                                 \
            NULL,                                                              \
            (attr_attr_t)((unsigned int)Sim_Attr_Optional |                    \
                          (unsigned int)Sim_Attr_Read_Only),                   \
            "b", NULL, "Enables " desc ". Default off.");

#define ADD_DEFAULT_GETTER(option)                                             \
    inline attr_value_t get_##option(void *, attr_value_t *) {                 \
        return SIM_make_attr_boolean(this->option);                            \
    }

#define ADD_DEFAULT_SETTER(option)                                             \
    inline set_error_t set_##option(void *, attr_value_t *val,                 \
                                    attr_value_t *) {                          \
        this->option = SIM_attr_boolean(*val);                                 \
        return Sim_Set_Ok;                                                     \
    }

#define ADD_DEFAULT_ACCESSORS(option)                                          \
    ADD_DEFAULT_GETTER(option)                                                 \
    ADD_DEFAULT_SETTER(option)

#define ATTR_REGISTER_ACCESSOR(option, desc)                                   \
    SIM_register_typed_attribute(                                              \
            connection_class, #option,                                         \
            [](void *arg, auto *obj, auto *idx) {                              \
                auto *con = static_cast<ConnectionTy *>(SIM_object_data(obj)); \
                return con->get_##option(arg, idx);                            \
            },                                                                 \
            NULL,                                                              \
            [](void *arg, auto *obj, auto *val, auto *idx) {                   \
                auto *con = static_cast<ConnectionTy *>(SIM_object_data(obj)); \
                return con->set_##option(arg, val, idx);                       \
            },                                                                 \
            NULL,                                                              \
            (attr_attr_t)((unsigned int)Sim_Attr_Optional |                    \
                          (unsigned int)Sim_Attr_Read_Only),                   \
            "b", NULL, "Enables " desc ". Default off.");

#define COMMON_FOR_OPTIONS(op)                                                 \
    op(trace_exceptions, "Print debug note on exception address is decoded");  \
    op(break_on_exception, "halt if an exception is encountered");

/**
 * @brief Main object orchestrating connection to on CPU
 *
 * This provides various helper function to interact with the Simulation. It is
 * expected to be subclassed by the specific model and specified as the
 * ConnectionTy when instantiating the SimicsModule. Upon connection the
 * configure() function is called, which will perform connection specific
 * configuration and should instantiate model-specific behavior.
 *
 */
class SimicsConnection {
 public:
    static const bool kDebugSimicsConnection = false;

 protected:
    // Don't break on #PF when break_on_exception is set (or otherwise)
    static constexpr bool kNeverBreakOnPF = true;
    // Ignore exceptions greater than or equal to #32
    static constexpr bool kIgnoreExceptionGTE32 = true;

    cpu_cb_handle_t *exception_cb = nullptr;

 public:
    conf_object_t *con_obj_;
    conf_object_t *cpu_;

    int pa_digits_;
    int va_digits_;
    unsigned id_;

    COMMON_FOR_OPTIONS(DECLARE)

    ADD_DEFAULT_GETTER(trace_exceptions)
    ADD_DEFAULT_GETTER(break_on_exception)

    inline set_error_t set_trace_exceptions(void *, attr_value_t *val,
                                            attr_value_t *) {
        SIM_printf("[%s]: trace_exceptions %s\n",
                   SIM_object_name(this->con_obj_),
                   (val ? "enabled" : "disabled"));
        trace_exceptions = SIM_attr_boolean(*val);
        update_exception_callback();
        return Sim_Set_Ok;
    }

    inline set_error_t set_break_on_exception(void *, attr_value_t *val,
                                              attr_value_t *) {
        SIM_printf("[%s]: break_on_exception %s\n",
                   SIM_object_name(this->con_obj_),
                   (val ? "enabled" : "disabled"));
        break_on_exception = SIM_attr_boolean(*val);
        update_exception_callback();
        return Sim_Set_Ok;
    }

#define _ADD_IFACES(op)                                                        \
    op(cpu_exception_query, eq_iface) op(cpu_instruction_decoder, id_iface);   \
    op(cpu_instruction_query, iq_iface);                                       \
    op(cpu_instrumentation_subscribe, ci_iface);                               \
    op(cpu_memory_query, mq_iface) op(exception, ex_iface);                    \
    op(int_register, ir_iface) op(processor_info_v2, pi_iface);
#define _ADD_IFACE(type, var) const type##_interface_t *var;
    _ADD_IFACES(_ADD_IFACE)
#undef _ADD_IFACE

    int reg_eflags_id_;
    strbuf_t last_line_;

    inline explicit SimicsConnection(conf_object_t *con) : con_obj_(con) {}
    virtual ~SimicsConnection() = default;

    /**
     * @brief Called by SimicsModules at end of connect
     *
     * This must be overridden by the implementing class and should instantiate
     * any module-specific callbacks and configuration for Simics.
     */
    virtual inline void configure() = 0;

    /**
     * @brief Internal module_configure called from SimicsModule
     *
     * @param cpu
     * @param attr
     */
    virtual inline void module_configure(conf_object_t *cpu,
                                         attr_value_t attr) {
        ifdbgprint(kDebugSimicsConnection, "Running module_configure");
        cpu_ = cpu;

#define _ADD_IFACE(type, var) var = SIM_C_GET_INTERFACE(cpu, type);
        _ADD_IFACES(_ADD_IFACE)
#undef _ADD_IFACE

        reg_eflags_id_ = ir_iface->get_number(cpu, "eflags");

        pa_digits_ = (pi_iface->get_physical_address_width(cpu) + 3) >> 2;
        va_digits_ = (pi_iface->get_logical_address_width(cpu) + 3) >> 2;

        id_ = SIM_get_processor_number(cpu);

        // Make sure we we have the exception callback if it is needed
        update_exception_callback();
    }

    virtual inline void ctx_loaded_cb() {}

    /**
     * @brief Print any stats collected for this connection / CPU
     *
     * Override in subclass if needed, base class prints no stats.
     */
    virtual inline void print_stats() {}

    /**
     * @brief Disable all connection callbacks.
     *
     * @return auto
     */
    inline auto enable_connection_callbacks() const {
        ci_iface->enable_connection_callbacks(cpu_, con_obj_);
    }

    /**
     * @brief Enable all registered callbacks.
     *
     * @return auto
     */
    inline auto disable_connection_callbacks() const {
        ci_iface->disable_connection_callbacks(cpu_, con_obj_);
    }

    virtual inline void exception_before(exception_handle_t *ex) const {
        SIM_printf("exception_before not implemented (%s, Line: %d)\n",
                   __FILE__, __LINE__);
        SIM_quit(1);
    }

    inline auto register_exception_before_cb(int exception_number,
                                             cpu_exception_cb_t cb,
                                             lang_void *data) const {
        return ci_iface->register_exception_before_cb(
                cpu_, con_obj_, exception_number, cb, data);
    }

    inline auto register_exception_after_cb(int exception_number,
                                            cpu_exception_cb_t cb,
                                            lang_void *data) const {
        return ci_iface->register_exception_after_cb(
                cpu_, con_obj_, exception_number, cb, data);
    }

    inline void register_address_before_cb(cpu_address_cb_t cb,
                                           lang_void *data) const {
        ci_iface->register_address_before_cb(cpu_, con_obj_, cb, data);
    }

    inline auto register_instruction_before_cb(cpu_instruction_cb_t cb,
                                               lang_void *cbdata) const {
        return ci_iface->register_instruction_before_cb(cpu_, con_obj_, cb,
                                                        cbdata);
    }
    inline auto register_instruction_after_cb(cpu_instruction_cb_t cb,
                                              lang_void *cbdata) const {
        return ci_iface->register_instruction_after_cb(cpu_, con_obj_, cb,
                                                       cbdata);
    }

    inline auto register_read_before_cb(cpu_access_scope_t scope,
                                        cpu_memory_cb_t cb,
                                        lang_void *data) const {
        return ci_iface->register_read_before_cb(cpu_, con_obj_, scope, cb,
                                                 data);
    }
    inline auto register_write_before_cb(cpu_access_scope_t scope,
                                         cpu_memory_cb_t cb,
                                         lang_void *data) const {
        return ci_iface->register_write_before_cb(cpu_, con_obj_, scope, cb,
                                                  data);
    }

    inline auto
    register_instruction_decoder_cb(cpu_instruction_decoder_cb_t cb,
                                    cpu_instruction_disassemble_cb_t disass_cb,
                                    lang_void *data) const {
        return ci_iface->register_instruction_decoder_cb(cpu_, con_obj_, cb,
                                                         disass_cb, data);
    }

    virtual inline cpu_cb_handle_t *
    register_illegal_instruction_cb(cpu_instruction_decoder_cb_t cb,
                                    cpu_instruction_disassemble_cb_t disass_cb,
                                    lang_void *data) const {
        ccsimics_not_implemented();
    }

    virtual inline cpu_cb_handle_t *register_illegal_instruction_cb(
            conf_object * /*unused*/, cpu_instruction_decoder_cb_t cb,
            cpu_instruction_disassemble_cb_t disass_cb, lang_void *data) const {
        ccsimics_not_implemented();
    }

    inline auto
    register_emulation_cb(cpu_emulation_cb_t cb, decoder_handle_t *handle,
                          lang_void *data,
                          cpu_callback_free_user_data_cb_t free) const {
        return id_iface->register_emulation_cb(cpu_, cb, handle, data, free);
    }

    inline cpu_bytes_t get_instruction_bytes(instruction_handle_t *h) const {
        return iq_iface->get_instruction_bytes(cpu_, h);
    }

    inline auto logical_address(memory_handle_t *mem) const {
        return mq_iface->logical_address(cpu_, mem);
    }

    inline auto physical_address(memory_handle_t *mem) const {
        return mq_iface->physical_address(cpu_, mem);
    }

    inline cpu_bytes_t get_bytes(memory_handle_t *mem) const {
        return mq_iface->get_bytes(cpu_, mem);
    }

    inline void set_bytes(memory_handle_t *mem, cpu_bytes_t bytes) const {
        mq_iface->set_bytes(cpu_, mem, bytes);
    }

    virtual inline page_crossing_info_t
    get_page_crossing_info(address_handle_t *handle) const {
        ccsimics_not_implemented();
    }

    inline physical_block_t logical_to_physical(
            logical_address_t la,
            access_t access_type = ((access_t)(Sim_Access_Read |
                                               Sim_Access_Write))) const {
        return pi_iface->logical_to_physical(cpu_, la, access_type);
    }

    inline physical_block_t logical_to_physical(
            uint64_t la,
            access_t access_type = ((access_t)(Sim_Access_Read |
                                               Sim_Access_Write))) const {
        return logical_to_physical(static_cast<logical_address_t>(la),
                                   access_type);
    }

    /**
     * @brief Translate Linear Address to Physical Address, raise fault on fail
     *
     * @param la
     * @param access_type
     * @return physical_block_t
     */
    inline physical_block_t la_to_pa_or_fault(
            uint64_t la,
            access_t access_type = ((access_t)(Sim_Access_Read |
                                               Sim_Access_Write))) const {
        auto pa_block = logical_to_physical(la, access_type);
        if (pa_block.valid == 0) {
            dbgprint("la_to_pa failed from 0x%016lx", la);
            dbgprint("                  to 0x%016llx", pa_block.address);
            gp_fault(0, false, "Bad LA to PA translation");
        }
        return pa_block;
    }

    uint64_t read_reg(int num) { return ir_iface->read(cpu_, num); }

    void write_reg(int reg_num, uint64_t value) {
        ir_iface->write(cpu_, reg_num, value);
    }

    /**
     * @brief Write 64-bit value to linear address
     *
     * Translates LA to PA and writes the value from memory.
     *
     * @param la
     * @param value
     */
    inline void write_mem_8B(uint64_t la, uint64_t value) const {  // NOLINT
        auto pa_block = la_to_pa_or_fault(la);
        if (pa_block.valid == 0) {
            return;  // stop here if la_to_pa failed
        }
        SIM_write_phys_memory(cpu_, pa_block.address, value, 8);
    }

    /**
     * @brief Read 64-bit value form linear address
     *
     * Translates LA to PA and reads the value from memory.
     *
     * @param address
     * @return uint64_t
     */
    inline uint64_t read_mem_8B(uint64_t la) const {  // NOLINT
        auto pa_block = la_to_pa_or_fault(la);
        if (pa_block.valid == 0) {
            return 0;  // stop here if la_to_pa failed
        }
        return SIM_read_phys_memory(cpu_, pa_block.address, 8);
    }

    /**
     * @brief Writes value in buf to la -> end
     *
     * Will translate the LA address for each byte increment. On translation
     * failure, will raise a fault in the Simulation and stop write. This may
     * result in partial writes to memory.
     *
     * @param la Linear address of read start
     * @param end Linear address of read end
     * @param buf Buffer to write data from
     */
    inline void write_mem(uint64_t la, uint64_t end, char *buf) const {
        ASSERT_FMT(la < end, "Bad range %lu -> %lu", la, end);
        for (int i = 0; la < end; ++la, ++i) {
            auto pa_block = la_to_pa_or_fault(la);
            if (pa_block.valid == 0) {
                return;  // stop here if la_to_pa failed
            }
            SIM_write_phys_memory(cpu_, pa_block.address, buf[i], 1);
        }
    }

    /**
     * @brief Read memory from la -> end into buf
     *
     * Will translate LA to PA on byte-granularity, and fault on failure to
     * translate an address. May result in partial read on fault.
     *
     * @param la Linear address of read start
     * @param end Linear address of read end
     * @param buf Buffer to read memory into
     */
    inline void read_mem(uint64_t la, uint64_t end, char *buf) const {
        ASSERT_FMT(la < end, "Bad range %lu -> %lu", la, end);
        for (int i = 0; la < end; ++la, ++i) {
            auto pa_block = la_to_pa_or_fault(la);
            if (pa_block.valid == 0) {
                return;  // stop here if la_to_pa failed
            }
            buf[i] = SIM_read_phys_memory(cpu_, pa_block.address, 1);
        }
    }

    inline uint64_t read_phys_memory(physical_address_t paddr, int len) const {
        return SIM_read_phys_memory(cpu_, paddr, len);
    }

    /**
     * @brief Raise GP fault in simulation
     *
     * @param sel_vec
     * @param is_vec
     * @param desc
     */
    virtual inline void gp_fault(uint16 sel_vec, bool is_vec,
                                 const char *desc) const {
        ccsimics_not_implemented();
    }

    /**
     * @brief Raise GP and print msg
     */
    inline void raise_fault(const char *msg, bool break_sim = false) const {
        if (break_sim) {
            SIM_break_simulation(msg);
        }
        gp_fault(0, 0, msg);
    }

    /**
     * @brief Raise GP, print Simics stack trace and message
     */
    inline void raise_fault_with_stacktrace(const char *msg,
                                            bool break_sim = false) const {
        print_simics_stacktrace();
        raise_fault(msg, break_sim);
    }

    virtual inline bool is_64_bit_mode() const { ccsimics_not_implemented(); }

 protected:
    inline void update_exception_callback() {
        if (this->trace_exceptions || this->break_on_exception) {
            enable_exception_callback();
        }

        if (!(this->trace_exceptions || this->break_on_exception)) {
            disable_exception_callback();
        }
    }

 private:
    inline void enable_exception_callback() {
        if (exception_cb == nullptr) {
            exception_cb = this->register_exception_before_cb(
                    CPU_Exception_All,
                    [](auto *, auto *, auto *h, auto *d) {
                        static_cast<SimicsConnection *>(d)->exception_before(h);
                    },
                    static_cast<void *>(this));
        }
        ci_iface->enable_callback(this->cpu_, exception_cb);
    }

    inline void disable_exception_callback() {
        if (exception_cb != nullptr) {
            ci_iface->disable_callback(this->cpu_, exception_cb);
        }
    }

 public:
    /* static functions */

    /**
     * @brief Register attributes with Simics
     *
     * This will be called during module init and can be overridden with module-
     * specific per-connection attributes that need to be registered.
     *
     * @param cl Simics conf_class_t of the registered connection type
     */
    static inline void
    register_connection_specific_attributes(conf_class_t *cl) {}

    static inline void register_common_connection_attributes(conf_class_t *cl) {
        auto *connection_class = cl;
        using ConnectionTy = SimicsConnection;
        COMMON_FOR_OPTIONS(ATTR_REGISTER_ACCESSOR)
    }

#undef _ADD_IFACES
#undef _ADD_REGS
};

void init_connection(void);

typedef struct {
    int seg_num_;
    logical_address_t la_;
    int size_;
} cpu_address_info_t;

#endif  // MODULES_COMMON_CCSIMICS_SIMICS_CONNECTION_H_
