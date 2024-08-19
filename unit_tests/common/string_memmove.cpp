// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: *
// xfail: -integrity-intra

// #define DEBUG

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>  // for va_list, va_start, va_end
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <gtest/gtest.h>
#include "unit_tests/common.h"

#define SRAND_SEED 0

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

TEST(STRING, mallocmemmove2) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t alloc_length = 2;

    buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
    for (int offset = 0; offset < 16; offset++) {
        str_offset = buff + offset;
        for (size_t length_expected = 1; length_expected < alloc_length;
             length_expected++) {
            fill_string(str_offset, length_expected);
            str = (char *)malloc(sizeof(char) * (length_expected + 1));
            memmove(str, str_offset, length_expected + 1);
            ASSERT_STREQ(str, str_offset);
            free(str);
        }
    }
    free(buff);
}

TEST(STRING, mallocmemmove8) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t alloc_length = 2;

    buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
    for (int offset = 0; offset < 16; offset++) {
        str_offset = buff + offset;
        for (size_t length_expected = 1; length_expected < alloc_length;
             length_expected++) {
            fill_string(str_offset, length_expected);
            str = (char *)malloc(sizeof(char) * (length_expected + 1));
            memmove(str, str_offset, length_expected + 1);
            ASSERT_STREQ(str, str_offset);
            free(str);
        }
    }
    free(buff);
}

TEST(STRING, mallocmemmove16) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t alloc_length = 2;

    buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
    for (int offset = 0; offset < 16; offset++) {
        str_offset = buff + offset;
        for (size_t length_expected = 1; length_expected < alloc_length;
             length_expected++) {
            fill_string(str_offset, length_expected);
            str = (char *)malloc(sizeof(char) * (length_expected + 1));
            memmove(str, str_offset, length_expected + 1);
            ASSERT_STREQ(str, str_offset);
            free(str);
        }
    }
    free(buff);
}

TEST(STRING, mallocmemmove128) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t alloc_length = 2;

    buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
    for (int offset = 0; offset < 16; offset++) {
        str_offset = buff + offset;
        for (size_t length_expected = 1; length_expected < alloc_length;
             length_expected++) {
            fill_string(str_offset, length_expected);
            str = (char *)malloc(sizeof(char) * (length_expected + 1));
            memmove(str, str_offset, length_expected + 1);
            ASSERT_STREQ(str, str_offset);
            free(str);
        }
    }
    free(buff);
}

TEST(STRING, mallocmemmoveX) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = 128;
    for (size_t alloc_length = 2; alloc_length < max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++) {
                dbg_dump_uint64(offset);
                dbg_dump_uint64(length_expected);
                dbg_dump_uint64(alloc_length);
                dbg_dump_uint64(buff);
                dbg_dump_uint64(str_offset);
                dbgprint("memmove");
                fill_string(str_offset, length_expected);
                dbgprint("malloc, %lu", length_expected + 1);
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                dbgprint("dbg_dumpuint_64");
                dbg_dump_uint64(str);
                dbgprint("memmove");
                memmove(str, str_offset, length_expected + 1);
                ASSERT_STREQ(str, str_offset);
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
