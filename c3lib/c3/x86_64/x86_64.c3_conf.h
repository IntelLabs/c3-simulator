// Copyright 2022-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef C3LIB_C3_X86_64_X86_64_C3_CONF_H_
#define C3LIB_C3_X86_64_X86_64_C3_CONF_H_

#include "c3/generic/c3_conf.h"
#include "c3/generic/defines.h"

static inline void cc_save_context(struct cc_context *ptr) {
    __asm__ __volatile__(".byte 0xf0; .byte 0x2f" : : "a"(ptr) : "memory");
}

static inline void cc_load_context(const struct cc_context *ptr) {
    __asm__ __volatile__(".byte 0xf0; .byte 0xfa" : : "a"(ptr) : "memory");
}

#endif  // C3LIB_C3_X86_64_X86_64_C3_CONF_H_
