// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: *
// skip: zts
// xfail: -integrity
// slow: yes
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
    for (size_t i = 0; i < max_size; i++) {
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
                printf("Mismatch on realloc:\n");
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

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
