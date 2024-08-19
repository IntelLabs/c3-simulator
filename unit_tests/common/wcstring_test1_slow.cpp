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

void fill_wcharstring(wchar_t *str, size_t size) {
    if (size != 0) {
        for (size_t i = 0; i < size; i++) {
            str[i] =
                    L'!' + i % (L'~' - L'!');  // pick any ascii between ! and ~
        }
    }
    str[size] = L'\0';
}

void fill_randomwcharstring(wchar_t *str, size_t size) {
    if (size != 0) {
        for (size_t i = 0; i < size; i++) {
            str[i] = L'*' + rand() % (L'z' - L'*');
        }
    }
    str[size] = L'\0';
}

TEST(STRING, mallocwcsspn) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    size_t max_length = 128;
    for (size_t alloc_length = 3; alloc_length < max_length; alloc_length++) {
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            fill_wcharstring(str_offset, alloc_length);
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++) {
                str = (wchar_t *)malloc(sizeof(wchar_t) *
                                        (length_expected + 1));
                fill_wcharstring(str, length_expected);
                ASSERT_EQ(wcsspn(str, str_offset), length_expected);
                free(str);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocwcpcpy) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    size_t max_length = 128;
    for (size_t alloc_length = 2; alloc_length < max_length; alloc_length++) {
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++) {
                fill_wcharstring(str_offset, length_expected);
                str = (wchar_t *)malloc(sizeof(wchar_t) *
                                        (length_expected + 1));
                wchar_t *ret = wcpcpy(str, str_offset);
                wchar_t value = *ret;
                ASSERT_STREQ(str, str_offset);
                ASSERT_EQ(value, L'\0');
                free(str);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocwcpncpy) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    size_t max_length = 128;
    for (size_t alloc_length = 5; alloc_length < max_length; alloc_length++) {
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 3; length_expected < alloc_length;
                 length_expected++) {
                fill_wcharstring(str_offset, length_expected);
                str = (wchar_t *)malloc(sizeof(wchar_t) *
                                        (length_expected + 1));
                wcpncpy(str, str_offset, length_expected - 2);
                str[length_expected - 2] = L'\0';
                ASSERT_STRNE(str, str_offset);
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