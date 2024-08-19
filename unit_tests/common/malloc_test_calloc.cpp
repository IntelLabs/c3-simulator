// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: *
// skip: zts
// xfail: -integrity
#include <malloc.h>
#include <xmmintrin.h>
#include <gtest/gtest.h>

#define DEBUG
#include "unit_tests/common.h"
// extern "C" {
// #include "encoding.h"
// }
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

static inline auto min(int a, int b) { return (a < b) ? a : b; }

TEST(Calloc, SingleAlloc64b) {
    uint64_t exp_val = 0x0;
    uint64_t *p = (uint64_t *)calloc(sizeof(uint64_t), 1);
    dbgprint("Checking %p -> %lu", p, *p);
    ASSERT_EQ(exp_val, *p);
    free(p);
}

TEST(Calloc, SingleAlloc32b) {
    uint32_t exp_val = 0x0;
    uint32_t *p = (uint32_t *)calloc(sizeof(uint32_t), 1);
    ASSERT_EQ(exp_val, *p);
    free(p);
}

TEST(Calloc, AllocRanging1Bto128M) {
    int max_size = 1 < 20;
    int max_num = 128;
    for (int size = 1; size <= max_size * max_num; size = size * max_num) {
        for (int num = 1; num <= max_num; num++) {
            uint8_t *p = (uint8_t *)calloc(num, size);
            // check for zero
            for (int ind = 0; ind < num * size; ind++) {
                ASSERT_EQ(0x0, p[ind]);
            }
            free(p);
        }
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
