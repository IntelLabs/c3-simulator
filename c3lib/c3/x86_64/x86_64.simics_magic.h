// Copyright 2016-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef C3LIB_C3_X86_64_X86_64_SIMICS_MAGIC_H_
#define C3LIB_C3_X86_64_X86_64_SIMICS_MAGIC_H_

#include "c3/generic/defines.h"

#ifndef MAGIC
#define MAGIC(n)                                                               \
    do {                                                                       \
        int simics_magic_instr_dummy;                                          \
        __asm__ __volatile__("cpuid"                                           \
                             : "=a"(simics_magic_instr_dummy)                  \
                             : "a"(0x4711 | ((unsigned)(n) << 16))             \
                             : "ecx", "edx", "ebx");                           \
    } while (0)
#endif

#endif  // C3LIB_C3_X86_64_X86_64_SIMICS_MAGIC_H_
