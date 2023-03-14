/*
 Copyright 2016 Intel Corporation
 SPDX-License-Identifier: MIT
*/
#include <simics/simulator/conf-object.h>
#include "ccsimics/simics_module.h"
#include "c3_simics_connection.h"  // NOLINT

class C3SimicsModule final
    : public SimicsModule<C3SimicsModule, C3SimicsConnection> {
 public:
    static constexpr const char *kModelName = "c3_model";
    static constexpr const char *kModelConnection = "c3_model_connection";
    static constexpr const char *kModelDescription = "C3";

    C3SimicsModule(conf_object_t *obj)
        : SimicsModule<C3SimicsModule, C3SimicsConnection>(obj) {}

    static void register_model_specific_attributes(conf_class_t *cl) {}
};

conf_class_t *connection_class = nullptr;

extern "C" {

void init_local() {
    C3SimicsModule::init_model();
    C3SimicsModule::init_connection();
}

}  // extern "C"
