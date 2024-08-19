// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: *
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

TEST(STRING, mallocwcscpy) {
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
                wcscpy(str, str_offset);
                ASSERT_STREQ(str, str_offset);
                free(str);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocwcscat) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str1, *str2;
    size_t max_length = 128;
    for (size_t alloc_length = 2; alloc_length < max_length; alloc_length++) {
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++) {
                fill_wcharstring(str_offset, length_expected);
                str1 = (wchar_t *)malloc(sizeof(wchar_t) *
                                         (length_expected + 1));
                fill_wcharstring(str1, length_expected);
                str2 = (wchar_t *)malloc(2 * sizeof(wchar_t) *
                                         (length_expected + 1));
                wcscpy(str2, str1);
                wcscat(str2, str_offset);
                ASSERT_STRNE(wcsstr(str2, str_offset), nullptr);
                ASSERT_STRNE(wcsstr(str2, str1), nullptr);
                free(str1);
                free(str2);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocwcsncat) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str1, *str2;
    size_t max_length = 128;
    for (size_t alloc_length = 2; alloc_length < max_length; alloc_length++) {
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++) {
                fill_wcharstring(str_offset, length_expected);
                str1 = (wchar_t *)malloc(sizeof(wchar_t) *
                                         (length_expected + 1));
                fill_wcharstring(str1, length_expected);
                str2 = (wchar_t *)malloc(sizeof(wchar_t) *
                                         (length_expected + 1) * 2);
                wcscpy(str2, str1);
                wcsncat(str2, str_offset, length_expected);
                ASSERT_STRNE(wcsstr(str2, str_offset), nullptr);
                ASSERT_STRNE(wcsstr(str2, str1), nullptr);
                free(str1);
                free(str2);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocwcscmp) {
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
                wcscpy(str, str_offset);
                ASSERT_EQ(wcscmp(str, str_offset), 0);
                free(str);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocwcsncmp) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    size_t max_length = 128;
    for (size_t alloc_length = 10; alloc_length < max_length; alloc_length++) {
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 4; length_expected < alloc_length;
                 length_expected++) {
                fill_wcharstring(str_offset, length_expected);
                str = (wchar_t *)malloc(sizeof(wchar_t) *
                                        (length_expected + 1));
                wcscpy(str, str_offset);

                // validating all 3 cases when length is less/equal/larger than
                // the actual string length
                ASSERT_EQ(wcsncmp(str, str_offset, length_expected), 0);
                ASSERT_EQ(wcsncmp(str, str_offset, length_expected / 2), 0);
                ASSERT_EQ(wcsncmp(str, str_offset, (length_expected + 10)), 0);

                free(str);
                str = (wchar_t *)malloc(sizeof(wchar_t) *
                                        (length_expected + 1));
                fill_randomwcharstring(str, length_expected);
                ASSERT_NE(wcsncmp(str, str_offset, length_expected / 2), 0);
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
