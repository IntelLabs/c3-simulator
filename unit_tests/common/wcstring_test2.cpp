// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: *
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

TEST(STRING, wcsstr) {
    srand(SRAND_SEED);
    wchar_t *str1;
    const wchar_t str2[] = L"abcdefgh";
    size_t str_length = 100;
    str1 = (wchar_t *)malloc(sizeof(wchar_t) * (str_length + 1));
    fill_wcharstring(str1, str_length);
    ASSERT_NE(wcsstr(str1, str2), nullptr);
    free(str1);
}

TEST(STRING, mallocwcsstr) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    size_t max_length = 128;
    for (size_t alloc_length = 5; alloc_length < max_length; alloc_length++) {
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            fill_wcharstring(str_offset, alloc_length);
            for (size_t length_expected = 3; length_expected < alloc_length;
                 length_expected++) {
                str = (wchar_t *)malloc(sizeof(wchar_t) *
                                        (length_expected + 1));
                fill_wcharstring(str, length_expected);
                ASSERT_NE(wcsstr(str_offset, str), nullptr);
                free(str);
                str = (wchar_t *)malloc(sizeof(wchar_t) *
                                        (length_expected + 1));
                fill_randomwcharstring(str, length_expected);
                ASSERT_EQ(wcsstr(str_offset, str), nullptr);
                free(str);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocwcsstrrandomsize) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    // Full 100 rounds in wcstring_test2_slow.cpp
    // for (size_t i = 1; i <= 100; i++) {
    for (size_t i = 1; i <= 10; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_wcharstring(str_offset, alloc_length);
        str = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 1));
        fill_wcharstring(str, alloc_length);
        ASSERT_NE(wcsstr(str_offset, str), nullptr);
        free(str);
        str = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 1));
        fill_randomwcharstring(str, alloc_length);
        ASSERT_EQ(wcsstr(str_offset, str), nullptr);
        free(str);
        free(buff);
    }
}

TEST(STRING, mallocwcschr) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    wchar_t ch = '!';
    size_t max_length = 128;
    for (size_t alloc_length = 5; alloc_length < max_length;
         alloc_length++, ch = '!') {
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++, ch = '!') {
            str_offset = buff + offset;
            for (size_t length_expected = 3; length_expected < alloc_length;
                 length_expected++, ch++) {
                fill_wcharstring(str_offset, length_expected);
                ASSERT_NE(wcschr(str_offset, ch), nullptr);
                if (ch == '|') {
                    ch = '!';
                }
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocwcschrrandomsize) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    wchar_t ch = '!';
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++, ch++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_wcharstring(str_offset, alloc_length);
        ASSERT_NE(wcschr(str_offset, ch), nullptr);
        if (ch == '|') {
            ch = '!';
        }
        free(buff);
    }
}

TEST(STRING, mallocwcschr1) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    wchar_t ch = ' ';
    size_t max_length = 128;
    for (size_t alloc_length = 3; alloc_length < max_length; alloc_length++) {
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 2; length_expected < alloc_length;
                 length_expected++) {
                fill_wcharstring(str_offset, length_expected);
                ASSERT_EQ(wcschr(str_offset, ch), nullptr);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocwcschr1randomsize) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    wchar_t ch = ' ';
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_wcharstring(str_offset, alloc_length);
        ASSERT_EQ(wcschr(str_offset, ch), nullptr);
    }
    free(buff);
}

TEST(STRING, mallocwcsrchr) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    wchar_t ch = '!';
    size_t max_length = 128;
    for (size_t alloc_length = 5; alloc_length < max_length;
         alloc_length++, ch = '!') {
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++, ch = '!') {
            str_offset = buff + offset;
            for (size_t length_expected = 3; length_expected < alloc_length;
                 length_expected++, ch++) {
                fill_wcharstring(str_offset, length_expected);
                ASSERT_NE(wcsrchr(str_offset, ch), nullptr);
                if (ch == '|') {
                    ch = '!';
                }
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocwcsrchrrandomsize) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    wchar_t ch = '!';
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++, ch++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_wcharstring(str_offset, alloc_length);
        ASSERT_NE(wcsrchr(str_offset, ch), nullptr);
        if (ch == '|') {
            ch = '!';
        }
        free(buff);
    }
}

TEST(STRING, mallocwcsrchr1) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    wchar_t ch = ' ';
    size_t max_length = 128;
    for (size_t alloc_length = 3; alloc_length < max_length; alloc_length++) {
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 2; length_expected < alloc_length;
                 length_expected++) {
                fill_wcharstring(str_offset, length_expected);
                ASSERT_EQ(wcsrchr(str_offset, ch), nullptr);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocwcsrchr1randomsize) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    wchar_t ch = ' ';
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_wcharstring(str_offset, alloc_length);
        ASSERT_EQ(wcsrchr(str_offset, ch), nullptr);
        free(buff);
    }
}

TEST(STRING, mallocwmemchr) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset;
    wchar_t ch = '!';
    size_t max_length = 128;
    for (size_t alloc_length = 5; alloc_length < max_length;
         alloc_length++, ch = '!') {
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++, ch = '!') {
            str_offset = buff + offset;
            for (size_t length_expected = 3; length_expected < alloc_length;
                 length_expected++, ch++) {
                fill_wcharstring(str_offset, length_expected);
                ASSERT_NE(wmemchr(str_offset, ch, length_expected), nullptr);
                if (ch == '|') {
                    ch = '!';
                }
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocwmemchrrandomsize) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset;
    wchar_t ch = '!';
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++, ch++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_wcharstring(str_offset, alloc_length);
        ASSERT_NE(wmemchr(str_offset, ch, alloc_length), nullptr);
        if (ch == '|') {
            ch = '!';
        }
        free(buff);
    }
}

TEST(STRING, mallocwmemchr1) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset;
    wchar_t ch = ' ';
    size_t max_length = 128;
    for (size_t alloc_length = 3; alloc_length < max_length; alloc_length++) {
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 2; length_expected < alloc_length;
                 length_expected++) {
                fill_wcharstring(str_offset, length_expected);
                ASSERT_EQ(wmemchr(str_offset, ch, length_expected), nullptr);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocwmemchr1randomsize) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset;
    wchar_t ch = ' ';
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_wcharstring(str_offset, alloc_length);
        ASSERT_EQ(wmemchr(str_offset, ch, alloc_length), nullptr);
        free(buff);
    }
}

TEST(STRING, mallocwcstof) {
    wchar_t str[] = L"20.345 16.398 36.40 12.810 6.93 809.76 573.29 37.04 "
                    L"783.206 19.45";
    wchar_t *ptr, *pend, *tmp;
    float res[10];
    float expval[] = {20.345, 16.398, 36.40, 12.810,  6.93,
                      809.76, 573.29, 37.04, 783.206, 19.45};
    ptr = (wchar_t *)malloc(sizeof(wchar_t) * (100));
    wcscpy(ptr, str);
    tmp = ptr;
    for (int i = 0; i < 10; i++) {
        res[i] = wcstof(tmp, &pend);
        tmp = pend;
        pend = NULL;
        EXPECT_EQ(expval[i], res[i]);
    }
    free(ptr);
}

TEST(STRING, mallocwcstod) {
    wchar_t str[] =
            L"20.345 16.39 20.08 6.93 809.76 573.29 37.04 783.206 19.45 12.810";
    wchar_t *ptr, *pend, *tmp;
    double res[10];
    double expval[] = {20.345, 16.39, 20.08,   6.93,  809.76,
                       573.29, 37.04, 783.206, 19.45, 12.810};
    ptr = (wchar_t *)malloc(sizeof(wchar_t) * (100));
    wcscpy(ptr, str);
    tmp = ptr;
    for (int i = 0; i < 10; i++) {
        res[i] = wcstod(tmp, &pend);
        tmp = pend;
        pend = NULL;
        EXPECT_EQ(expval[i], res[i]);
    }
    free(ptr);
}

TEST(STRING, mallocwcstol) {
    wchar_t str[] = L"101 202 456 178 563 635 264 3654 354 236";
    wchar_t *ptr, *pend, *tmp;
    long res[10];                                // NOLINT
    long expval[] = {101, 202, 456,  178, 563,   // NOLINT
                     635, 264, 3654, 354, 236};  // NOLINT
    ptr = (wchar_t *)malloc(sizeof(wchar_t) * (100));
    wcscpy(ptr, str);
    tmp = ptr;
    for (int i = 0; i < 10; i++) {
        res[i] = wcstol(tmp, &pend, 10);
        tmp = pend;
        pend = NULL;
        EXPECT_EQ(expval[i], res[i]);
    }
    free(ptr);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
