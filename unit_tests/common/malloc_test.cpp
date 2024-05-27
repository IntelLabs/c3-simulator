// model: *
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
TEST(Malloc, ZeroAlloc) {
    uint8_t *p = (uint8_t *)malloc(0);
    ASSERT_GE(malloc_usable_size(p), 0);
    free(p);
}
TEST(Malloc, SingleAlloc) {
    uint32_t exp_val = 0xFFFFFFFF;
    uint32_t *p = (uint32_t *)malloc(sizeof(uint32_t));
    *p = exp_val;
    ASSERT_EQ(exp_val, *p);
}
TEST(Malloc, SmallAlloc) {
    srand(0);
    const int num = 100;
    size_t max_size = 16;
    uint8_t *p[num];
    size_t size_arr[num];
    for (int i = 0; i < num; i++) {
        size_arr[i] = rand() % (max_size + 1);
        p[i] = (uint8_t *)malloc(size_arr[i]);
        for (size_t j = 0; j < size_arr[i]; j++) {
            p[i][j] = (uint8_t)i;
        }
    }
    for (int i = 0; i < num; i++) {
        for (size_t j = 0; j < size_arr[i]; j++) {
            ASSERT_EQ(p[i][j], (uint8_t)i);
        }
        free(p[i]);
    }
}

TEST(Malloc, AllocThen_8B_to_64B_Reads) {
    int max_size = 1000;
    srand(0);
    uint8_t exp_val8[max_size];
    uint8_t *p8 = (uint8_t *)malloc(max_size);
    // for (int offset = 0; offset < 64; offset+)
    for (int i = 0; i < max_size; i++) {
        exp_val8[i] = (uint8_t)rand();
        p8[i] = exp_val8[i];
    }

    for (int i8 = 0; i8 < max_size; i8++) {
        ASSERT_EQ(exp_val8[i8], p8[i8]);
    }
    uint16_t *exp_val16 = (uint16_t *)exp_val8;
    uint16_t *p16 = (uint16_t *)p8;
    for (int i16 = 0; i16 < max_size / 2; i16++) {
        ASSERT_EQ(exp_val16[i16], p16[i16]);
    }
    uint32_t *exp_val32 = (uint32_t *)exp_val8;
    uint32_t *p32 = (uint32_t *)p8;
    for (int i32 = 0; i32 < max_size / 4; i32++) {
        ASSERT_EQ(exp_val32[i32], p32[i32]);
    }
    uint64_t *exp_val64 = (uint64_t *)exp_val8;
    uint64_t *p64 = (uint64_t *)p8;
    for (int i64 = 0; i64 < max_size / 8; i64++) {
        ASSERT_EQ(exp_val64[i64], p64[i64]);
    }
    free(p8);
}
TEST(Malloc, AllocThen_8B_to_64B_ReadsWithOffset) {
    int max_size = 1000;
    srand(0);
    uint8_t exp_val8[max_size];
    uint8_t *p8 = (uint8_t *)malloc(max_size);
    for (int i = 0; i < max_size; i++) {
        exp_val8[i] = (uint8_t)rand();
        p8[i] = exp_val8[i];
    }
    for (int offset = 1; offset < 64; offset++) {
        uint16_t *exp_val16 = (uint16_t *)(exp_val8 + offset);
        uint16_t *p16 = (uint16_t *)(p8 + offset);
        for (int i16 = 0; i16 < (max_size - offset) / 2; i16++) {
            ASSERT_EQ(exp_val16[i16], p16[i16]);
        }
        uint32_t *exp_val32 = (uint32_t *)(exp_val8 + offset);
        uint32_t *p32 = (uint32_t *)(p8 + offset);
        for (int i32 = 0; i32 < (max_size - offset) / 4; i32++) {
            ASSERT_EQ(exp_val32[i32], p32[i32]);
        }
        uint64_t *exp_val64 = (uint64_t *)(exp_val8 + offset);
        uint64_t *p64 = (uint64_t *)(p8 + offset);
        for (int i64 = 0; i64 < (max_size - offset) / 8; i64++) {
            ASSERT_EQ(exp_val64[i64], p64[i64]);
        }
    }
    free(p8);
}
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

TEST(Utils, MallocUsableSize) {
    srand(1234);
    size_t max_size = 10000;
    for (unsigned int i = 0; i < max_size; i++) {
        size_t size = 1 + (((size_t)rand()) % max_size);
        uint8_t *p = (uint8_t *)malloc(size);
        ASSERT_LE(size, malloc_usable_size(p));
        free(p);
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
