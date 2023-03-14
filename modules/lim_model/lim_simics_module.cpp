/*
 Copyright 2021 Intel Corporation
 SPDX-License-Identifier: MIT
*/
#include "lim_simics_module.h"      // NOLINT
#include "lim_simics_connection.h"  // NOLINT
#include "model.h"                  // NOLINT

conf_class_t *connection_class = nullptr;

int trace_only = false;
int lim_no_encode = 0;

extern "C" {
void init_local() {
    LimSimicsModule::init_model();
    LimSimicsModule::init_connection();
}

const char *__asan_default_options() { return c3_asan_default_options(); }

}  // extern "C"
