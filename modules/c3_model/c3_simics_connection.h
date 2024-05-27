/*
 Copyright 2016 Intel Corporation
 SPDX-License-Identifier: MIT
*/
#ifndef MODULES_C3_MODEL_C3_SIMICS_CONNECTION_H_
#define MODULES_C3_MODEL_C3_SIMICS_CONNECTION_H_

#include <memory>
#include <simics/base/conf-object.h>
#include "ccsimics/context.h"
#include "ccsimics/integrity.h"
#include "ccsimics/integrity_isa.h"
#include "ccsimics/ptrencdec_isa.h"
#include "ccsimics/simics_connection.h"
#include "c3_model.h"  // NOLINT


#define FOR_OPTIONS_INTERNAL(op)


#define FOR_OPTIONS(op)                                                        \
    op(debug_on, "enable debug messages");                                     \
    op(break_on_decode_fault,                                                  \
       "halt simulation when a non-canonical address is decoded");             \
    op(disable_data_encryption,                                                \
       "disable data encryption (but keep pointer encoding)");                 \
    op(integrity, "enable integrity");                                         \
    op(stack_hardening, "enable stack hardening");                             \
    FOR_OPTIONS_INTERNAL(op)

class C3SimicsConnection;
using C3Context = ContextFinal<C3SimicsConnection>;

#define CC_SIMICS_POINTER_ENCODING_TYPE CCPointerEncoding


using C3PtrEnc = CC_SIMICS_POINTER_ENCODING_TYPE;
using C3ModelTy = ccsimics::C3Model<C3SimicsConnection, C3Context, C3PtrEnc>;

class C3PtrencdecIsa final
    : public ccsimics::PtrencdecIsa<C3SimicsConnection, C3ModelTy> {
 public:
    C3PtrencdecIsa(C3SimicsConnection *con, C3ModelTy *model)
        : ccsimics::PtrencdecIsa<C3SimicsConnection, C3ModelTy>(con, model) {}
};

class C3SimicsConnection final : public SimicsConnection {
    using CtxTy = C3Context;
    using ConTy = C3SimicsConnection;
    using integrity_isa_t =
            ccsimics::IntegrityIsa<C3SimicsConnection, C3ModelTy>;

 public:
    FOR_OPTIONS(DECLARE)

    std::unique_ptr<C3ModelTy> c3_;
    std::unique_ptr<C3Context> ctx_;
    std::unique_ptr<C3PtrEnc> ptr_enc_;
    std::unique_ptr<C3PtrencdecIsa> ptr_encdec_isa_;
    std::unique_ptr<integrity_isa_t> integrityisa_;

    C3SimicsConnection(conf_object_t *con) : SimicsConnection(con) {}
    virtual ~C3SimicsConnection() = default;

    ADD_DEFAULT_ACCESSORS(break_on_decode_fault)
    ADD_DEFAULT_ACCESSORS(disable_data_encryption)
    ADD_DEFAULT_GETTER(debug_on)
    ADD_DEFAULT_GETTER(integrity)
    ADD_DEFAULT_GETTER(stack_hardening)

    void configure() {
        ptr_enc_ = std::make_unique<C3PtrEnc>(this);

        ctx_ = std::make_unique<C3Context>(this, ptr_enc_.get());
        ctx_->enable<C3Context>();

        c3_ = std::make_unique<C3ModelTy>(this, ctx_.get(), ptr_enc_.get());
        c3_->register_callbacks<C3ModelTy>(this);

        ptr_encdec_isa_ = std::make_unique<C3PtrencdecIsa>(this, c3_.get());
        ptr_encdec_isa_->register_callbacks(this->con_obj_);

        integrityisa_ = std::make_unique<integrity_isa_t>(this, c3_.get());
        integrityisa_->register_callbacks(this->con_obj_);

        // Set configuration options
        c3_->set_integrity(this->integrity);
        set_debug_on(this->debug_on);
    }

    inline void set_debug_on(bool val) {
        if (val != this->debug_on) {
            this->debug_on = val;
            SIM_printf("DEBUG messages %s\n", val ? "enabled" : "disabled");
        }

        // We may not have called configure yet, so check for NULL
        if (ptr_encdec_isa_) {
            ptr_encdec_isa_->m_debug_ = val;
        }
    }

    inline C3ModelTy *get_model() { return this->c3_.get(); }

    inline auto *get_integrity() { return this->c3_->get_integrity(); }

    inline void set_integrity(bool val) {
        SIM_printf("[C3] Integrity %s\n", (val ? "enabled" : "disabled"));
        this->integrity = val;
        if (c3_) {
            c3_->set_integrity(integrity);
        }
    }

    inline set_error_t set_debug_on(void *, attr_value_t *v, attr_value_t *) {
        set_debug_on(SIM_attr_boolean(*v));
        return Sim_Set_Ok;
    }

    inline set_error_t set_integrity(void *, attr_value_t *v, attr_value_t *) {
        set_integrity(SIM_attr_boolean(*v));
        return Sim_Set_Ok;
    }

    inline set_error_t set_stack_hardening(void *, attr_value_t *val,
                                           attr_value_t *) {
        SIM_printf("[CC] Stack hardening %s\n", (val ? "enabled" : "disabled"));
        stack_hardening = SIM_attr_boolean(*val);
        if (c3_) {
            c3_->set_stack_hardening(stack_hardening);
        }
        return Sim_Set_Ok;
    }

    void print_stats() override { c3_->print_stats(); }

    inline void ctx_loaded_cb() override {
        this->c3_->set_integrity(this->ctx_->get_icv_enabled());
        if (this->ctx_->get_and_zero_icv_map_reset()) {
            this->c3_->clear_icv_map();
        }
    }

    static inline void
    register_connection_specific_attributes(conf_class_t *cl) {
        auto *connection_class = cl;
        using ConnectionTy = C3SimicsConnection;
        FOR_OPTIONS(ATTR_REGISTER_ACCESSOR)

        C3PtrencdecIsa::register_attributes(cl);
        C3ModelTy::register_attributes(cl);
        C3Context::register_attributes<C3SimicsConnection>(cl);
    }

    inline auto *get_ptrencdec_isa() { return this->ptr_encdec_isa_.get(); }
    inline auto *get_integrity_isa() { return this->integrityisa_.get(); }
};

#endif  // MODULES_C3_MODEL_C3_SIMICS_CONNECTION_H_
