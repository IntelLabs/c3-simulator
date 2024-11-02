// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef MODULES_COMMON_CCSIMICS_RISCV_SIMICS_CONNECTION_H_
#define MODULES_COMMON_CCSIMICS_RISCV_SIMICS_CONNECTION_H_

#include "ccsimics/simics_connection.h"

class RiscvSimicsConnection : public SimicsConnection {
 public:
    inline explicit RiscvSimicsConnection(conf_object_t *con)
        : SimicsConnection(con) {}
    virtual ~RiscvSimicsConnection() = default;

#define _ADD_IFACES(op)

// Declare reg_id variables (defined in constructor)
#define _ADD_REGS(op) op(pc)
#define _ADD_REG(reg) int reg##_id;
    _ADD_REGS(_ADD_REG)
#undef _ADD_REG

    virtual inline void module_configure(conf_object_t *cpu,
                                         attr_value_t attr) {
        ifdbgprint(kDebugSimicsConnection,
                   "Calling SimicsConnection::module_configure");
        SimicsConnection::module_configure(cpu, attr);

        ifdbgprint(kDebugSimicsConnection, "Running module_configure");

        ifdbgprint(kDebugSimicsConnection, "Setting up register info");
        ASSERT_MSG(ir_iface != nullptr, "ir_iface is null");
#define _ADD_REG(reg) reg##_id = ir_iface->get_number(cpu, #reg);
        _ADD_REGS(_ADD_REG)
#undef _ADD_REG
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
};

#endif  // MODULES_COMMON_CCSIMICS_RISCV_SIMICS_CONNECTION_H_
