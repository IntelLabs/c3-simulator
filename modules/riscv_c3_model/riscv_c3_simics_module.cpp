// Copyright 2016-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#include <simics/simulator/conf-object.h>
#include "ccsimics/simics_module.h"
#include "riscv_c3_simics_connection.h"  // NOLINT

namespace ccsimics {

class RiscvC3SimicsModule final
    : public SimicsModule<RiscvC3SimicsModule, RiscvC3SimicsConnection> {
 public:
    static constexpr const char *kModelName = "riscv_c3_model";
    static constexpr const char *kModelConnection = "riscv_c3_model_connection";
    static constexpr const char *kModelDescription = "riscv_c3";

    RiscvC3SimicsModule(conf_object_t *obj)
        : SimicsModule<RiscvC3SimicsModule, RiscvC3SimicsConnection>(obj) {}

    static void register_model_specific_attributes(conf_class_t *cl) {}
};

}  // namespace ccsimics

conf_class_t *connection_class = nullptr;

extern "C" {

void init_local() {
    ccsimics::RiscvC3SimicsModule::init_model();
    ccsimics::RiscvC3SimicsModule::init_connection();
}

}  // extern "C"
