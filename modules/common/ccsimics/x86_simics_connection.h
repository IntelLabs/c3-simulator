// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef MODULES_COMMON_CCSIMICS_X86_SIMICS_CONNECTION_H_
#define MODULES_COMMON_CCSIMICS_X86_SIMICS_CONNECTION_H_

#include <simics/arch/x86-instrumentation.h>
#include <simics/arch/x86.h>
#include "ccsimics/simics_connection.h"

class X86SimicsConnection : public SimicsConnection {
 public:
    inline explicit X86SimicsConnection(conf_object_t *con)
        : SimicsConnection(con) {}
    virtual ~X86SimicsConnection() = default;

#define _ADD_IFACES(op)                                                        \
    op(smm_instrumentation_subscribe, smm_iface);                              \
    op(x86_address_query, x86_aq_iface);                                       \
    op(x86_exception, x86_ex_iface);                                           \
    op(x86_exception_query, x86_eq_iface);                                     \
    op(x86_instruction_query, x86_iq_iface);                                   \
    op(x86_instrumentation_subscribe, x86_iface);                              \
    op(x86_memory_query, x86_mq_iface);                                        \
    op(x86_reg_access, x86_reg_access_iface);
#define _ADD_IFACE(type, var) const type##_interface_t *var;
    _ADD_IFACES(_ADD_IFACE)
#undef _ADD_IFACE

// Declare reg_id variables (defined in constructor)
#define _ADD_REGS(op) op(rip) op(rsp) op(rax) op(rdx) op(rdi)
#define _ADD_REG(reg) int reg##_id;
    _ADD_REGS(_ADD_REG)
#undef _ADD_REG

    virtual inline void module_configure(conf_object_t *cpu,
                                         attr_value_t attr) {
        ifdbgprint(kDebugSimicsConnection,
                   "Calling SimicsConnection::module_configure");
        SimicsConnection::module_configure(cpu, attr);

        ifdbgprint(kDebugSimicsConnection, "Running module_configure");

        ifdbgprint(kDebugSimicsConnection, "Setting up interfaces");
#define _ADD_IFACE(type, var) var = SIM_C_GET_INTERFACE(cpu, type);
        _ADD_IFACES(_ADD_IFACE)
#undef _ADD_IFACE

        ifdbgprint(kDebugSimicsConnection, "Setting up register info");
        ASSERT_MSG(ir_iface != nullptr, "ir_iface is null");
#define _ADD_REG(reg) reg##_id = ir_iface->get_number(cpu, #reg);
        _ADD_REGS(_ADD_REG)
#undef _ADD_REG

        if (x86_iface == nullptr) {
            SIM_LOG_ERROR(cpu, 0,
                          "%s does not implement"
                          " x86_instrumentation_subscribe interface",
                          SIM_object_name(cpu));
        }
    }

    auto *register_smm_enter_before_cb(smm_switch_cb_t cb,
                                       lang_void *user_data) {
        return smm_iface->register_smm_enter_before_cb(cpu_, con_obj_, cb,
                                                       user_data);
    }

    auto *register_smm_enter_after_cb(smm_switch_cb_t cb,
                                      lang_void *user_data) {
        return smm_iface->register_smm_enter_after_cb(cpu_, con_obj_, cb,
                                                      user_data);
    }

    auto *register_smm_leave_before_cb(smm_switch_cb_t cb,
                                       lang_void *user_data) {
        return smm_iface->register_smm_leave_before_cb(cpu_, con_obj_, cb,
                                                       user_data);
    }

    auto *register_smm_leave_after_cb(smm_switch_cb_t cb,
                                      lang_void *user_data) {
        return smm_iface->register_smm_leave_after_cb(cpu_, con_obj_, cb,
                                                      user_data);
    }

    inline cpu_cb_handle_t *
    register_illegal_instruction_cb(cpu_instruction_decoder_cb_t cb,
                                    cpu_instruction_disassemble_cb_t disass_cb,
                                    lang_void *data) const override {
        return x86_iface->register_illegal_instruction_cb(
                this->cpu_, this->con_obj_, cb, disass_cb, data);
    }

    inline cpu_cb_handle_t *
    register_illegal_instruction_cb(conf_object * /*unused*/,
                                    cpu_instruction_decoder_cb_t cb,
                                    cpu_instruction_disassemble_cb_t disass_cb,
                                    lang_void *data) const override {
        return register_illegal_instruction_cb(cb, disass_cb, data);
    }

    // inline cpu_cb_handle_t *register_mode_switch_cb(x86_mode_switch_cb_t cb,
    //                                     lang_void *data) const {
    //     return x86_iface->register_mode_switch_cb(this->cpu_, this->con_obj_,
    //     cb, data);
    // }

    inline page_crossing_info_t
    get_page_crossing_info(address_handle_t *handle) const override {
        return x86_aq_iface->get_page_crossing_info(this->cpu_, handle);
    }

    /**
     * @brief Raise GP fault in simulation
     *
     * @param sel_vec
     * @param is_vec
     * @param desc
     */
    inline void gp_fault(uint16 sel_vec, bool is_vec,
                         const char *desc) const override {
        x86_ex_iface->GP_fault(cpu_, sel_vec, is_vec, desc);
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

    inline uint64_t read_rip() const { return ir_iface->read(cpu_, rip_id); }
    inline uint64_t read_rsp() const { return ir_iface->read(cpu_, rsp_id); }
    inline uint64_t read_rax() const { return ir_iface->read(cpu_, rax_id); }
    inline uint64_t read_rdx() const { return ir_iface->read(cpu_, rdx_id); }
    inline uint64_t read_rdi() const { return ir_iface->read(cpu_, rdi_id); }
    inline uint64_t read_eflags() const {
        return ir_iface->read(cpu_, reg_eflags_id_);
    }
    inline void write_rip(uint64_t val) { ir_iface->write(cpu_, rip_id, val); }
    inline void write_rsp(uint64_t val) { ir_iface->write(cpu_, rsp_id, val); }
    inline void write_rax(uint64_t val) { ir_iface->write(cpu_, rax_id, val); }
    inline void write_rdx(uint64_t val) { ir_iface->write(cpu_, rdx_id, val); }
    inline void write_rdi(uint64_t val) { ir_iface->write(cpu_, rdi_id, val); }

    inline x86_exec_mode_t get_exec_mode() const {
        return x86_reg_access_iface->get_exec_mode(cpu_);
    }

    inline x86_xmode_info_t get_xmode_info() const {
        return x86_reg_access_iface->get_xmode_info(cpu_);
    }

    inline bool is_64_bit_mode() const override {
        return (this->get_exec_mode() == x86_exec_mode_t::X86_Exec_Mode_64);
    }

    inline uint64_t get_gpr(const int gpr) const {
        return x86_reg_access_iface->get_gpr(cpu_, gpr);
    }

    inline uint64_t get_cr(const x86_cr_t cr) const {
        return x86_reg_access_iface->get_cr(cpu_, cr);
    }

    inline x86_seg_reg_t get_seg(const x86_seg_t seg) const {
        return x86_reg_access_iface->get_seg(cpu_, seg);
    }

    inline bool is_user_mode() const {
        return (get_seg(X86_Seg_CS).base & 3) == 0;
    }

    inline void set_gpr(const int gpr, const uint64_t val) const {
        return x86_reg_access_iface->set_gpr(cpu_, gpr, val);
    }

    inline uint8_t get_exception_vector(exception_handle_t *eh) const {
        return x86_eq_iface->vector(cpu_, eh);
    }

    inline void exception_before(exception_handle_t *eh) const override {
        if (this->trace_exceptions || this->break_on_exception) {
            const auto e = this->get_exception_vector(eh);
            if (!kIgnoreExceptionGTE32 || e < 32) {
                SIM_printf(
                        "Encountered exception #%d (source: %s)\n", e,
                        exception_source_2str(this->get_exception_source(eh)));
                if (this->break_on_exception && (!kNeverBreakOnPF || e != 14)) {
                    SIM_printf("Breaking on exception\n");
                    SIM_break_simulation("Breaking on exception\n");
                }
            }
        }
    }

    inline x86_exception_source_t
    get_exception_source(exception_handle_t *eh) const {
        return x86_eq_iface->source(cpu_, eh);
    }

    inline uint32_t get_exception_error_code(exception_handle_t *eh) const {
        return x86_eq_iface->source(cpu_, eh);
    }

 protected:
    inline void update_exception_callback();

 private:
    inline void enable_exception_callback();
    inline void disable_exception_callback();

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

#endif  // MODULES_COMMON_CCSIMICS_X86_SIMICS_CONNECTION_H_
