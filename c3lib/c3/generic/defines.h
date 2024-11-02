// Copyright 2016-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef C3LIB_C3_GENERIC_DEFINES_H_
#define C3LIB_C3_GENERIC_DEFINES_H_

#if !defined(C3_X86_64) && !defined(C3_RISC_V)
#define C3_X86_64
#endif

#ifdef C3_RISC_V
#define CC_NO_SHADOW_RIP_ENABLE
#define CC_NO_INTEGRITY_ENABLE
#define CC_NO_ZTS_ENABLE
#define CC_NO_ICV_ENABLE
#endif

#ifndef _CC_GLOBALS_NO_INCLUDES_
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#endif  // !_CC_GLOBALS_NO_INCLUDES_

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

#ifndef CC_NO_SHADOW_RIP_ENABLE
#define CC_SHADOW_RIP_ENABLE
#endif

#ifndef CC_NO_INTEGRITY_ENABLE
#define CC_INTEGRITY_ENABLE
#endif

#ifndef CC_NO_ZTS_ENABLE
#endif

#ifndef CC_NO_ICV_ENABLE
#define CC_ICV_ENABLE
#endif

#define CC_DS_REP_MOVS_ENABLE

#define C3_BITMASK(x) (0xFFFFFFFFFFFFFFFF >> (64 - (x)))

#define MAGIC_LIT_START 1
#define MAGIC_LIT_END 2
#define MAGIC_MALLOC_ENTER 3
#define MAGIC_MALLOC_EXIT 4
#define MAGIC_CALLOC_ENTER 5
#define MAGIC_CALLOC_EXIT 6
#define MAGIC_REALLOC_ENTER 7
#define MAGIC_REALLOC_EXIT 8
#define MAGIC_FREE_ENTER 9
#define MAGIC_FREE_EXIT 10

// INTRA-object magic value
#define MAGIC_VAL_INTRA 0xdeadbeefd0d0caca

#define BOX_PAD_FOR_STRLEN_FIX 48

// Use a prime number for depth for optimum results
#define QUARANTINE_DEPTH 373
#define QUARANTINE_WIDTH 2

#endif  // C3LIB_C3_GENERIC_DEFINES_H_
