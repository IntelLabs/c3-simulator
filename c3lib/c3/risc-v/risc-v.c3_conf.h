// Copyright 2022-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef C3LIB_C3_RISC_V_RISC_V_C3_CONF_H_
#define C3LIB_C3_RISC_V_RISC_V_C3_CONF_H_

#include "c3/generic/c3_conf.h"
#include "c3/generic/defines.h"

static inline void cc_save_context(struct cc_context *ptr) {
    asm("BAD INSTRUCTION, NOT IMPLEMENTED");
}

static inline void cc_load_context(const struct cc_context *ptr) {
    asm("BAD INSTRUCTION, NOT IMPLEMENTED");
}

#endif  // C3LIB_C3_RISC_V_RISC_V_C3_CONF_H_
