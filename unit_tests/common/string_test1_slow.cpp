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

#define SRAND_SEED 0

void fill_string(char *str, size_t size) {
    if (size != 0) {
        for (size_t i = 0; i < size; i++) {
            str[i] = '!' + i % ('~' - '!');  // pick any ascii between ! and ~
        }
    }
    str[size - 1] = '\0';
}

void fill_numbers(char *str, size_t size) {
    if (size != 0) {
        for (size_t i = 0; i < size; i++) {
            str[i] = '0' + i % ('9' - '0');
        }
    }
    str[size - 1] = '\0';
}

void fill_randomnumbers(char *str, size_t size) {
    if (size != 0) {
        for (size_t i = 0; i < size; i++) {
            str[i] = '0' + rand() % ('9' - '0');
        }
    }
    str[size - 1] = '\0';
}

void fill_randomalphabets(char *str, size_t size) {
    if (size != 0) {
        for (size_t i = 0; i < size; i++) {
            str[i] = 'a' + rand() % ('z' - 'a');
        }
    }
    str[size - 1] = '\0';
}

void fill_randomstring(char *str, size_t size) {
    if (size != 0) {
        for (size_t i = 0; i < size; i++) {
            str[i] = '*' + rand() % ('z' - '*');
        }
    }
    str[size - 1] = '\0';
}

TEST(STRING, mallocstrpbrk) {
    char *buff, *str_offset, *keys;
    size_t max_length = 128;
    size_t length_expected;
    for (size_t alloc_length = 5; alloc_length <= max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (max_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (length_expected = 3; length_expected < alloc_length;
                 length_expected++) {
                fill_string(str_offset, max_length);
                keys = (char *)malloc(sizeof(char) * (length_expected + 1));
                fill_randomstring(keys, length_expected);
                ASSERT_NE(strpbrk(str_offset, keys), nullptr);
                free(keys);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocstrspn) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str, *str1;
    size_t max_length = 128;
    for (size_t alloc_length = 3; alloc_length < max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++) {
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                if (length_expected >= 10) {
                    fill_randomnumbers(str_offset, length_expected + 1);
                    str1 = str_offset + length_expected;
                    fill_string(str1, alloc_length - length_expected + 1);
                    fill_numbers(str, length_expected + 1);
                } else {
                    fill_string(str_offset, alloc_length + 1);
                    fill_string(str, length_expected + 1);
                }
                ASSERT_EQ(strspn(str_offset, str), length_expected);
                free(str);
                fill_string(str_offset, alloc_length + 1);
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                fill_randomstring(str, length_expected + 1);
                ASSERT_EQ(strspn(str_offset, str), 0);
                free(str);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocstrcspn) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str, *str1;
    size_t max_length = 128;
    for (size_t alloc_length = 1; alloc_length <= max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++) {
                fill_randomalphabets(str_offset, length_expected);
                str1 = str_offset + length_expected;
                fill_numbers(str1, alloc_length - length_expected);
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                fill_numbers(str, length_expected + 1);
                ASSERT_EQ(strcspn(str_offset, str), length_expected - 1);
                free(str);
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                fill_string(str_offset, alloc_length);
                fill_string(str, length_expected + 1);
                ASSERT_EQ(strcspn(str_offset, str), 0);
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
