// Copyright 2021-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef C3LIB_C3_X86_64_X86_64_CA_H_
#define C3LIB_C3_X86_64_X86_64_CA_H_

#include "c3/generic/ca.h"
#include "c3/generic/defines.h"

static inline uint64_t cc_isa_encptr(uint64_t ptr, const ptr_metadata_t *md) {
    asm(".byte 0xf0			\n"
        ".byte 0x48			\n"
        ".byte 0x01			\n"
        ".byte 0xc8         \n"
        : [ptr] "+a"(ptr)
        : [md] "c"((uint64_t)md->uint64_)
        :);
    return ptr;
}

static inline uint64_t cc_isa_decptr(uint64_t pointer) {
    asm(".byte 0xf0   		\n"
        ".byte 0x48			\n"
        ".byte 0x01			\n"
        ".byte 0xc0         \n"
        : [ptr] "+a"(pointer)
        :
        :);
    return pointer;
}

#endif  // C3LIB_C3_X86_64_X86_64_CA_H_
