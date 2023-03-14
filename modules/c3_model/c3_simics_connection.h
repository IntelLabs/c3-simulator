/*
 Copyright 2016 Intel Corporation
 SPDX-License-Identifier: MIT
*/
#ifndef MODULES_C3_MODEL_C3_SIMICS_CONNECTION_H_
#define MODULES_C3_MODEL_C3_SIMICS_CONNECTION_H_

#include <memory>
#include <simics/base/conf-object.h>
#include "ccsimics/c3_model.h"
#include "ccsimics/context.h"
#include "ccsimics/ptrencdec_isa.h"
#include "ccsimics/simics_connection.h"

#define FOR_OPTIONS(op)                                                        \
    op(debug_on, "enable debug messages");                                     \
    op(break_on_decode_fault,                                                  \
       "halt simulation when a non-canonical address is decoded");             \
    op(disable_data_encryption,                                                \
       "disable data encryption (but keep pointer encoding)");

class C3SimicsConnection;

class C3PtrEnc final : public CCPointerEncoding {
 public:
    C3PtrEnc() {}
};

class C3Context final : public Context {
 public:
    C3Context(SimicsConnection *con, C3PtrEnc *ptr_enc)
        : Context(con, ptr_enc) {}
};

class Model final : public C3Model<C3SimicsConnection, C3Context, C3PtrEnc> {
 public:
    Model(C3SimicsConnection *con, C3Context *ctx, C3PtrEnc *ptr_enc)
        : C3Model<C3SimicsConnection, C3Context, C3PtrEnc>(con, ctx, ptr_enc) {}
};

class C3PtrencdecIsa final
    : public ccsimics::PtrencdecIsa<C3SimicsConnection, Model> {
 public:
    C3PtrencdecIsa(C3SimicsConnection *con, Model *model)
        : ccsimics::PtrencdecIsa<C3SimicsConnection, Model>(con, model) {}
};

class C3SimicsConnection final : public SimicsConnection {
 public:
    FOR_OPTIONS(DECLARE)

    std::unique_ptr<Model> c3_;
    std::unique_ptr<C3Context> ctx_;
    std::unique_ptr<C3PtrEnc> ptr_enc_;
    std::unique_ptr<C3PtrencdecIsa> ptr_encdec_isa_;

    C3SimicsConnection(conf_object_t *con) : SimicsConnection(con) {}
    virtual ~C3SimicsConnection() = default;

    void configure() {
        ptr_enc_ = std::make_unique<C3PtrEnc>();

        ctx_ = std::make_unique<C3Context>(this, ptr_enc_.get());
        ctx_->enable<C3Context>();

        c3_ = std::make_unique<Model>(this, ctx_.get(), ptr_enc_.get());
        c3_->register_callbacks<Model>(this);

        ptr_encdec_isa_ = std::make_unique<C3PtrencdecIsa>(this, c3_.get());
        ptr_encdec_isa_->register_callbacks(this->con_obj_);

        if (this->debug_on) {
            SIM_printf("DEBUG messages enabled\n");
        }
    }

    void print_stats() override { c3_->print_stats(); }

    static inline void
    register_connection_specific_attributes(conf_class_t *cl) {
        auto *connection_class = cl;
        using ConnectionTy = C3SimicsConnection;
        FOR_OPTIONS(ATTR_REGISTER)
    }
};

#endif  // MODULES_C3_MODEL_C3_SIMICS_CONNECTION_H_
