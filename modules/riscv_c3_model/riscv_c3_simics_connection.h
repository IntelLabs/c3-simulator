// Copyright 2016-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef MODULES_RISCV_C3_MODEL_RISCV_C3_SIMICS_CONNECTION_H_
#define MODULES_RISCV_C3_MODEL_RISCV_C3_SIMICS_CONNECTION_H_

#include <memory>
#include <simics/base/conf-object.h>
#include "ccsimics/context.h"
#include "ccsimics/riscv_context.h"
#include "ccsimics/riscv_simics_connection.h"
#include "ccsimics/simics_connection.h"
#include "riscv_c3_model.h"  // NOLINT

#define FOR_OPTIONS_INTERNAL(op)

#define FOR_OPTIONS(op)                                                        \
    op(debug_on, "enable debug messages");                                     \
    op(break_on_decode_fault,                                                  \
       "halt simulation when a non-canonical address is decoded");             \
    op(disable_data_encryption,                                                \
       "disable data encryption (but keep pointer encoding)");                 \
    FOR_OPTIONS_INTERNAL(op)

class RiscvC3SimicsConnection;
using RiscvC3Context = ContextFinal<RiscvC3SimicsConnection, RiscvContext>;

using RiscvC3PtrEnc = CCPointerEncoding;
using RiscvC3ModelTy = ccsimics::RiscvC3Model<RiscvC3SimicsConnection,
                                              RiscvC3Context, RiscvC3PtrEnc>;

class RiscvC3SimicsConnection final : public RiscvSimicsConnection {
    using CtxTy = RiscvC3Context;
    using ConTy = RiscvC3SimicsConnection;

 public:
    FOR_OPTIONS(DECLARE)

    std::unique_ptr<RiscvC3ModelTy> c3_;
    std::unique_ptr<RiscvC3Context> ctx_;
    std::unique_ptr<RiscvC3PtrEnc> ptr_enc_;

    RiscvC3SimicsConnection(conf_object_t *con) : RiscvSimicsConnection(con) {}
    virtual ~RiscvC3SimicsConnection() = default;

    ADD_DEFAULT_ACCESSORS(break_on_decode_fault)
    ADD_DEFAULT_ACCESSORS(disable_data_encryption)
    ADD_DEFAULT_GETTER(debug_on)

    void configure() {
        dbgprint("Here we go 1...");
        ptr_enc_ = std::make_unique<RiscvC3PtrEnc>(this);

        dbgprint("Here we go 2...");
        ctx_ = std::make_unique<RiscvC3Context>(this, ptr_enc_.get());
        ctx_->enable<RiscvC3Context>();

        dbgprint("Here we go 3...");
        c3_ = std::make_unique<RiscvC3ModelTy>(this, ctx_.get(),
                                               ptr_enc_.get());
        c3_->register_callbacks<RiscvC3ModelTy>(this);

        // Set configuration options
        dbgprint("Here we go 4...");
        set_debug_on(this->debug_on);
    }

    inline void set_debug_on(bool val) {
        if (val != this->debug_on) {
            this->debug_on = val;
            SIM_printf("DEBUG messages %s\n", val ? "enabled" : "disabled");
        }
    }

    inline RiscvC3ModelTy *get_model() { return this->c3_.get(); }

    inline set_error_t set_debug_on(void *, attr_value_t *v, attr_value_t *) {
        set_debug_on(SIM_attr_boolean(*v));
        return Sim_Set_Ok;
    }

    void print_stats() override { c3_->print_stats(); }

    static inline void
    register_connection_specific_attributes(conf_class_t *cl) {
        auto *connection_class = cl;
        using ConnectionTy = RiscvC3SimicsConnection;
        FOR_OPTIONS(ATTR_REGISTER_ACCESSOR)

        RiscvC3ModelTy::register_attributes(cl);
        RiscvC3Context::register_attributes<RiscvC3SimicsConnection>(cl);
    }
};

#endif  // MODULES_RISCV_C3_MODEL_RISCV_C3_SIMICS_CONNECTION_H_
