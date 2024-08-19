// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: *
// slow: yes
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>  // for va_list, va_start, va_end
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <gtest/gtest.h>

#define MAGIC(n)                                                               \
    do {                                                                       \
        int simics_magic_instr_dummy;                                          \
        __asm__ __volatile__("cpuid"                                           \
                             : "=a"(simics_magic_instr_dummy)                  \
                             : "a"(0x4711 | ((unsigned)(n) << 16))             \
                             : "ecx", "edx", "ebx");                           \
    } while (0)

#define SRAND_SEED 0

// Helper function to init ref data buffer with random data
/*
static void init_reference_data_random(uint8_t * buffer, int num) {
    srand(SRAND_SEED);
    for (int i = 0; i<num; i++) {
        buffer[i] = (uint8_t) rand();
    }
}
*/

static void init_reference_data_sequential(uint8_t *buffer, int num) {
    uint8_t sequence = 0x0;
    for (int i = 0; i < num; i++) {
        buffer[i] = sequence++;
    }
}

/*
static void init_reference_data_0x11(uint8_t * buffer, int num) {
    for (int i = 0; i<num; i++) {
        buffer[i] = 0x11;
    }
}
*/
/*
TEST(MEMCPY, Case4849) {
    int max_size = 1 << 20;
    int ref_size = max_size;
    uint8_t ref_data[ref_size];
    init_reference_data_0x11(ref_data, ref_size);
    int bytes = 4849;
    uint8_t* p = (uint8_t*) calloc (1, bytes);
    memcpy((void*) p, (void*) ref_data, bytes);
    for (int i = 0; i < bytes; i++) {
        if (ref_data[i] != p[i]){
            MAGIC(0);
            printf("ERROR: Mismatch detected:\n");
            printf("num bytes=%d, i=%d, p=0x%016lx\n", bytes, i, (uint64_t) p);
            ASSERT_EQ(ref_data[i], p[i]);
        }
    }
    free(p);
}
*/

size_t BLI_snprintf(char *dst, size_t maxncpy, const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    size_t n = vsnprintf(dst, maxncpy, format, arg);
    va_end(arg);
    return n;
}
void fill_string(char *str, size_t size) {
    if (size != 0) {
        for (size_t i = 0; i < size; i++) {
            str[i] = '!' + i % ('~' - '!');  // pick any ascii between ! and ~
        }
    }
    str[size] = '\0';
}

void fill_randomstring(char *str, size_t size) {
    if (size != 0) {
        for (size_t i = 0; i < size; i++) {
            str[i] = '*' + rand() % ('z' - '*');
        }
    }
    str[size] = '\0';
}

TEST(STRING, mallocstrcat) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str1, *str2;
    size_t max_length = 128;
    for (size_t alloc_length = 2; alloc_length < max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++) {
                fill_string(str_offset, length_expected);
                str1 = (char *)malloc(sizeof(char) * (length_expected + 1));
                fill_string(str1, length_expected);
                str2 = (char *)malloc(2 * sizeof(char) * (length_expected + 1));
                strcpy(str2, str1);
                strcat(str2, str_offset);
                ASSERT_STRNE(strstr(str2, str_offset), nullptr);
                ASSERT_STRNE(strstr(str2, str2), nullptr);
                free(str1);
                free(str2);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocstrncat) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str1, *str2;
    size_t max_length = 128;
    for (size_t alloc_length = 2; alloc_length < max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++) {
                fill_string(str_offset, length_expected);
                str1 = (char *)malloc(sizeof(char) * (length_expected + 1));
                fill_string(str1, length_expected);
                str2 = (char *)malloc(2 * sizeof(char) * (length_expected + 1));
                strcpy(str2, str1);
                strncat(str2, str_offset, length_expected);
                ASSERT_STRNE(strstr(str2, str_offset), nullptr);
                ASSERT_STRNE(strstr(str2, str1), nullptr);
                free(str1);
                free(str2);
            }
        }
        free(buff);
    }
}

TEST(MEMCPY, CopyNBytesFromRefToBuffer) {
    int max_size = 1 << 16;
    int ref_size = max_size;
    uint8_t ref_data[ref_size];
    init_reference_data_sequential(ref_data, ref_size);
    for (int bytes = 0; bytes < (1 << 13); bytes++) {
        uint8_t *p = (uint8_t *)calloc(1, bytes);
        /*
        if (bytes==4856){
            printf("num bytes=%d, p=0x%016lx\n", bytes, (uint64_t) p);
            printf("&p[4840] = 0x%016lx\n",  (uint64_t) &p[4840]);
            printf("MAGIC BREAKPOINT before memcpy\n");
            MAGIC(0);
            memcpy((void*) p, (void*) ref_data, bytes);
            printf("MAGIC BREAKPOINT after memcpy\n");
            MAGIC(0);
        } else {
            memcpy((void*) p, (void*) ref_data, bytes);
        }
        */
        memcpy((void *)p, (void *)ref_data, bytes);
        for (int i = 0; i < bytes; i++) {
            if (ref_data[i] != p[i]) {
                printf("ERROR: Mismatch detected:\n");
                printf("num bytes=%d, i=%d, p=0x%016lx\n", bytes, i,
                       (uint64_t)p);
                printf("MAGIC BREAKPOINT after mismatch\n");
                MAGIC(0);
                ASSERT_EQ(ref_data[i], p[i]);
            }
        }
        free(p);
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
