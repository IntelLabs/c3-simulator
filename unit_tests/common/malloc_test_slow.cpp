// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: *
// slow: yes
// skip: zts
// xfail: -integrity
#include <malloc.h>
#include <xmmintrin.h>
#include <gtest/gtest.h>
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

TEST(Malloc, AllocThen_128B_Accesses) {
    srand(123456);
    const int N = 3000;
    uint8_t ref_data[N];
    for (int i = 0; i < N; i++) {
        ref_data[i] = (uint8_t)rand();
    }
    void *allocs[N];
    for (int i = 0; i < N; i++) {
        int size =
                sizeof(__m128) + (((int)rand()) % (N - (sizeof(__m128) - 1)));
        assert(size <= N);
        uint8_t *p = (uint8_t *)malloc(size);
        allocs[i] = p;
        for (int j = 0; j < N; j++) {
            int offset = rand() % (size - (sizeof(__m128) - 1));

            assert(offset + sizeof(__m128) <= size);
            float *flt_ref_data = (float *)(ref_data + offset);
            float *flt_p = (float *)(p + offset);

            __m128 ref_val = _mm_set_ps(flt_ref_data[0], flt_ref_data[1],
                                        flt_ref_data[2], flt_ref_data[3]);

            // store using an SSE instruction:
            _mm_storeu_ps(flt_p, ref_val);

            // load using an SSE instruction:
            __m128 ld_val1 = _mm_loadu_ps(flt_p);

            assert(memcmp(&ld_val1, &ref_val, sizeof(ref_val)) == 0);

            // store using non-SSE instructions:
            for (int k = 0; k < sizeof(__m128) / sizeof(float); k++)
                flt_p[((sizeof(__m128) / sizeof(float)) - 1) - k] =
                        flt_ref_data[k];

            // load using an SSE instruction:
            __m128 ld_val2 = _mm_loadu_ps(flt_p);

            assert(memcmp(&ld_val2, &ref_val, sizeof(ref_val)) == 0);

            // load using non-SSE instructions:
            float ld_flt[sizeof(__m128) / sizeof(float)];
            for (int k = 0; k < sizeof(__m128) / sizeof(float); k++)
                ld_flt[k] = flt_p[((sizeof(__m128) / sizeof(float)) - 1) - k];

            __m128 ld_flt_val =
                    _mm_set_ps(ld_flt[0], ld_flt[1], ld_flt[2], ld_flt[3]);

            assert(memcmp(&ld_flt_val, &ref_val, sizeof(ref_val)) == 0);
        }
    }
    // Free all the allocations at the end to result in a complex memory layout
    // during the test.
    for (int i = 0; i < N; i++) {
        free(allocs[i]);
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
