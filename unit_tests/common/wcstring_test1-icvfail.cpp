// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: *

// This test had previous issues with ICV mismatches, not investigated further.

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

// Causes ICV failures
TEST(STRING, mallocwcstok) {
    srand(SRAND_SEED);
    wchar_t *str, *buff, *str_offset, *tstr;
    wchar_t delim[] = L"}";
    size_t str_length = 93;
    size_t max_length = 128;
    str = (wchar_t *)malloc(sizeof(wchar_t) * max_length);
    fill_wcharstring(str, str_length - 1);
    for (size_t alloc_length = 5; alloc_length < max_length; alloc_length++) {
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 3; length_expected < alloc_length;
                 length_expected++) {
                wchar_t *ptr, *token;
                str_offset = (wchar_t *)malloc(sizeof(wchar_t) *
                                               (length_expected + 1));
                fill_wcharstring(str_offset, length_expected);
                tstr = (wchar_t *)malloc(sizeof(wchar_t) *
                                         (length_expected + 1));
                fill_wcharstring(tstr, length_expected);
                token = wcstok(str_offset, delim, &ptr);
                while (token) {
                    if (length_expected <= (str_length - 1)) {
                        ASSERT_STREQ(tstr, str_offset);
                    } else {
                        ASSERT_STREQ(str, str_offset);
                    }
                    token = wcstok(NULL, delim, &ptr);
                }
                free(tstr);
            }
        }
        free(buff);
    }
    free(str);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
