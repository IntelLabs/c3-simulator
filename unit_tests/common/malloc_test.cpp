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

TEST(Calloc, SingleAlloc) {
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
TEST(Realloc, SmallToLarge) {
    // pointer_key_t crypto_key;
    // init_crypto_key_struct(&crypto_key);
    int small_size = 8;
    int large_size = 1 << 30;
    // init reference data
    int ref_size = small_size;
    uint8_t ref_data[ref_size];
    for (int i = 0; i < ref_size; i++) {
        ref_data[i] = (uint8_t)rand();
    }
    uint8_t *p = (uint8_t *)malloc(small_size);
    // ASSERT_FALSE(is_canonical((uint64_t) p));
    // init p with reference data
    for (int i = 0; i < ref_size; i++) {
        p[i] = ref_data[i];
    }
    uint8_t *p_new = (uint8_t *)realloc(p, large_size);
    // ASSERT_TRUE(is_canonical((uint64_t) p_new));
    // check whether old data survived
    for (int i = 0; i < ref_size; i++) {
        ASSERT_EQ(ref_data[i], p_new[i]);
    }
    free(p_new);
}
TEST(Realloc, LargeToSmall) {
    // pointer_key_t crypto_key;
    // init_crypto_key_struct(&crypto_key);
    int small_size = 8;
    int large_size = 1 << 30;
    // init reference data
    int ref_size = small_size;
    uint8_t ref_data[ref_size];
    for (int i = 0; i < ref_size; i++) {
        ref_data[i] = (uint8_t)rand();
    }
    uint8_t *p = (uint8_t *)malloc(large_size);
    // ASSERT_TRUE(is_canonical((uint64_t) p));
    // init p with reference data
    for (int i = 0; i < ref_size; i++) {
        p[i] = ref_data[i];
    }
    uint8_t *p_new = (uint8_t *)realloc(p, small_size);
    // ASSERT_FALSE(is_canonical((uint64_t) p_new));
    // check whether old data survived
    for (int i = 0; i < ref_size; i++) {
        ASSERT_EQ(ref_data[i], p_new[i]);
    }
    free(p_new);
}
TEST(Realloc, SmallToExtraLarge) {
    size_t small_size = 8;
    size_t large_size = 1ULL << 32;
    // init reference data
    size_t ref_size = small_size;
    uint8_t ref_data[ref_size];
    for (size_t i = 0; i < ref_size; i++) {
        ref_data[i] = (uint8_t)rand();
    }
    uint8_t *p = (uint8_t *)malloc(small_size);
    for (size_t i = 0; i < ref_size; i++) {
        p[i] = ref_data[i];
    }
    uint8_t *p_new = (uint8_t *)realloc(p, large_size);
    // check whether old data survived
    for (size_t i = 0; i < ref_size; i++) {
        ASSERT_EQ(ref_data[i], p_new[i]);
    }
    free(p_new);
}
TEST(Realloc, ExtraLargeToSmall) {
    size_t small_size = 8;
    size_t large_size = 1ULL << 32;
    // init reference data
    size_t ref_size = small_size;
    uint8_t ref_data[ref_size];
    for (size_t i = 0; i < ref_size; i++) {
        ref_data[i] = (uint8_t)rand();
    }
    uint8_t *p = (uint8_t *)malloc(large_size);
    for (size_t i = 0; i < ref_size; i++) {
        p[i] = ref_data[i];
    }
    uint8_t *p_new = (uint8_t *)realloc(p, small_size);
    // check whether old data survived
    for (size_t i = 0; i < ref_size; i++) {
        ASSERT_EQ(ref_data[i], p_new[i]);
    }
    free(p_new);
}

TEST(Realloc, CheckOldData_LargeAlloc) {
    // pointer_key_t crypto_key;
    // init_crypto_key_struct(&crypto_key);
    srand(123456);
    size_t max_size = 10000;
    uint8_t ref_data[max_size];
    for (size_t i = 0; i < 10000; i++) {
        size_t size = 1 + (((int)rand()) % max_size);
        for (size_t i = 0; i < size; i++) {
            ref_data[i] = (uint8_t)rand();
        }
        uint8_t *p = (uint8_t *)malloc(size);
        // if(decoding_ptr_with_offset_mismatch ((uint64_t)p, size-1,
        // &crypto_key)) { printf("GTEST: violating p\n"); printf("GTEST:
        // size_old=%d\n", (int)size); ASSERT_TRUE(0);
        //}
        // uint64_t p_decoded = decode_pointer((uint64_t)p, &crypto_key);
        for (int offset = 0; offset < size; offset++) {
            p[offset] = ref_data[offset];
        }
        size_t new_size = 1 + (((int)rand()) % max_size);
        // printf("************ iteration i=%d ************\n", i);
        // printf("GTEST: size = %d bytes, new_size = %d bytes\n", (int) size,
        // (int) new_size);
        uint8_t *p_new = (uint8_t *)realloc((void *)p, new_size);
        if (p_new == NULL) {
            printf("GTEST: p_new is NULL\n");
            printf("GTEST: size    =%d\n", (int)size);
            printf("GTEST: new_size=%d\n", (int)new_size);
            ASSERT_TRUE(0);
        }
        // uint64_t p_new_decoded = decode_pointer((uint64_t)p_new,
        // &crypto_key); if(decoding_ptr_with_offset_mismatch ((uint64_t)p_new,
        // new_size, &crypto_key)) { printf ("GTEST: violating p_new\n"); printf
        // ("GTEST: size_old=%d, size_new=%d\n", (int)size, (int) new_size);
        // ASSERT_TRUE(0);
        //}
        for (int offset = 0; offset < min(size, new_size); offset++) {
            int match = (ref_data[offset] == p_new[offset]) ? 1 : 0;
            if (!match) {
                printf("i=%d\n", i);
                printf("Missmatch on realloc:\n");
                printf("size    =%d\n", (int)size);
                printf("new_size=%d\n", (int)new_size);
                printf("offset  =%d\n", (int)offset);
                printf("ref_data  =0x%02x\n", ref_data[offset]);
                printf("p_new     =0x%02x\n", p_new[offset]);
            }
            ASSERT_TRUE(match);
        }
        free((void *)p_new);
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
