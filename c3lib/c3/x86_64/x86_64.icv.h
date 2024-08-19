// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef C3LIB_C3_X86_64_X86_64_ICV_H_
#define C3LIB_C3_X86_64_X86_64_ICV_H_

#include "c3/generic/defines.h"

#ifdef CC_INTEGRITY_ENABLE

static inline void cc_isa_invicv(uint64_t pointer) {
    asm volatile(".byte 0xf0   		\n"
                 ".byte 0x48			\n"
                 ".byte 0x2B			\n"
                 ".byte 0xc0         \n"
                 :
                 : [ptr] "a"(pointer)
                 :);
}

static inline void cc_isa_initicv(const uint64_t ptr, const uint64_t data) {
    asm volatile(".byte 0xf0             \n"
                 ".byte 0x48             \n"
                 ".byte 0x21             \n"
                 ".byte 0xc8             \n"
                 :
                 : [ptr] "a"(ptr), [val_ptr] "c"(data)
                 :);
}

static inline void cc_isa_preiniticv(const uint64_t ptr) {
    asm volatile(".byte 0xf0             \n"
                 ".byte 0x48             \n"
                 ".byte 0x21             \n"
                 ".byte 0xc0             \n"
                 :
                 : [ptr] "a"(ptr)
                 :);
}

#endif  // CC_INTEGRITY_ENABLE
#endif  // C3LIB_C3_X86_64_X86_64_ICV_H_
