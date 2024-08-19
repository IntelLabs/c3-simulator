// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: *
// slow: yes
// xfail: -integrity-intra
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>  // for va_list, va_start, va_end
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <gtest/gtest.h>

#define SRAND_SEED 0

#define MAX_LENGTH 128

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

TEST(STRING, mallocmemmem) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = MAX_LENGTH;
    for (size_t alloc_length = 5; alloc_length < max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            fill_string(str_offset, alloc_length);
            for (size_t length_expected = 3; length_expected < alloc_length;
                 length_expected++) {
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                fill_string(str, length_expected);
                ASSERT_NE(
                        memmem(str_offset, alloc_length, str, length_expected),
                        nullptr);
                free(str);
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                fill_randomstring(str, length_expected);
                ASSERT_EQ(
                        memmem(str_offset, alloc_length, str, length_expected),
                        nullptr);
                free(str);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocmemcmp) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = MAX_LENGTH;
    for (size_t alloc_length = 10; alloc_length < max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 4; length_expected < alloc_length;
                 length_expected++) {
                fill_string(str_offset, length_expected);
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                memcpy(str, str_offset, length_expected + 1);
                ASSERT_EQ(memcmp(str, str_offset, length_expected + 1), 0);
                free(str);
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                fill_randomstring(str, length_expected);
                ASSERT_NE(memcmp(str, str_offset, length_expected + 1), 0);
                free(str);
            }
        }
        free(buff);
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
