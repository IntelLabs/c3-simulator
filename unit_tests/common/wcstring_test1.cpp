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

TEST(STRING, mallocwcscspn) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    size_t max_length = 128;
    for (size_t alloc_length = 10; alloc_length < max_length; alloc_length++) {
        for (int offset = 0; offset < 16; offset++) {
            buff = (wchar_t *)malloc(sizeof(wchar_t) * (max_length + 17));
            str_offset = buff + offset;
            fill_wcharstring(str_offset, max_length);
            str = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 1));
            fill_randomwcharstring(str, alloc_length);
            ASSERT_NE(wcscspn(str, str_offset), max_length);
            free(str);
            free(buff);
        }
    }
}

TEST(STRING, mallocwcscspnrandomsize) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (wchar_t *)malloc(sizeof(wchar_t) *
                                 (alloc_length + offset + 101));
        str_offset = buff + offset;
        fill_wcharstring(str_offset, alloc_length + 100);
        str = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 1));
        fill_randomwcharstring(str, alloc_length);
        ASSERT_NE(wcscspn(str, str_offset), alloc_length + 100);
        free(str);
        free(buff);
    }
}

TEST(STRING, mallocwcspbrk) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    size_t max_length = 128;
    for (size_t alloc_length = 5; alloc_length <= max_length; alloc_length++) {
        for (int offset = 0; offset < 16; offset++) {
            buff = (wchar_t *)malloc(sizeof(wchar_t) * (max_length + 17));
            str_offset = buff + offset;
            fill_wcharstring(str_offset, max_length);
            str = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 1));
            fill_randomwcharstring(str, alloc_length);
            ASSERT_NE(wcspbrk(str_offset, str), nullptr);
            free(str);
            free(buff);
        }
    }
}

TEST(STRING, mallocwcspbrkrandomsize) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (wchar_t *)malloc(sizeof(wchar_t) *
                                 (alloc_length + offset + 101));
        str_offset = buff + offset;
        fill_wcharstring(str_offset, alloc_length + 100);
        str = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 1));
        fill_randomwcharstring(str, alloc_length);
        ASSERT_NE(wcspbrk(str_offset, str), nullptr);
        free(str);
        free(buff);
    }
}

TEST(STRING, mallocwcsspn) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    size_t max_length = 128;
    // Full test relocated to wcstring_test1_slow.cpp
    max_length = 48;
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

TEST(STRING, mallocwcsspnrandomsize) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (wchar_t *)malloc(sizeof(wchar_t) *
                                 (alloc_length + offset + 101));
        str_offset = buff + offset;
        fill_wcharstring(str_offset, alloc_length + 100);
        str = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 1));
        fill_wcharstring(str, alloc_length);
        ASSERT_EQ(wcsspn(str, str_offset), alloc_length);
        free(str);
        free(buff);
    }
}

TEST(STRING, mallocwcpcpy) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    size_t max_length = 128;
    // Full test relocated to wcstring_test1_slow.cpp
    max_length = 48;
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

TEST(STRING, mallocwcpcpyrandomsize) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_wcharstring(str_offset, alloc_length);
        str = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 1));
        wchar_t *ret = wcpcpy(str, str_offset);
        wchar_t value = *ret;
        ASSERT_STREQ(str, str_offset);
        ASSERT_EQ(value, L'\0');
        free(str);
        free(buff);
    }
}

TEST(STRING, mallocwcpncpy) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    size_t max_length = 128;
    // Full test relocated to wcstring_test1_slow.cpp
    max_length = 48;
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

TEST(STRING, mallocwcpncpyrandomsize) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_wcharstring(str_offset, alloc_length);
        str = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 1));
        wcpncpy(str, str_offset, alloc_length - 2);
        str[alloc_length - 2] = L'\0';
        ASSERT_STRNE(str, str_offset);
        free(str);
        free(buff);
    }
}

TEST(STRING, mallocwcrtomb) {
    setlocale(LC_ALL, "en_US.utf8");
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset;
    char *outstr;
    size_t max_length = 128;
    int returnval;
    for (size_t alloc_length = 1; alloc_length < max_length; alloc_length++) {
        for (int offset = 0; offset < 16; offset++) {
            buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 17));
            str_offset = buff + offset;
            fill_wcharstring(str_offset, alloc_length);
            outstr = (char *)malloc(sizeof(char) * (alloc_length + 1));
            mbstate_t ps = mbstate_t();
            for (size_t length_expected = 0;
                 length_expected < wcslen(str_offset); length_expected++) {
                returnval = wcrtomb(outstr, str_offset[length_expected], &ps);
                EXPECT_EQ(returnval, 1);
            }
            free(outstr);
            free(buff);
        }
    }
}

TEST(STRING, mallocwcrtombrandomsize) {
    setlocale(LC_ALL, "en_US.utf8");
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset;
    char *outstr;
    size_t max_length = 4096 * 4;
    int returnval;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_wcharstring(str_offset, alloc_length);
        outstr = (char *)malloc(sizeof(char) * (alloc_length + 1));
        mbstate_t ps = mbstate_t();
        returnval = wcrtomb(outstr, str_offset[alloc_length], &ps);
        EXPECT_EQ(returnval, 1);
        free(outstr);
        free(buff);
    }
}

TEST(STRING, mallocwcstokrandomsize) {
    srand(SRAND_SEED);
    wchar_t *str, *buff, *str_offset, *tstr;
    wchar_t delim[] = L"}";
    size_t str_length = 93;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    str = (wchar_t *)malloc(sizeof(wchar_t) * max_length);
    fill_wcharstring(str, str_length - 1);
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        wchar_t *ptr, *token;
        str_offset = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 1));
        fill_wcharstring(str_offset, alloc_length);
        tstr = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 1));
        fill_wcharstring(tstr, alloc_length);
        token = wcstok(str_offset, delim, &ptr);
        while (token) {
            if (alloc_length <= (str_length - 1)) {
                ASSERT_STREQ(tstr, str_offset);
            } else {
                ASSERT_STREQ(str, str_offset);
            }
            token = wcstok(NULL, delim, &ptr);
        }
        free(buff);
        free(tstr);
    }
    free(str);
}

TEST(STRING, mallocwmemset) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset;
    wchar_t ch = L'a';
    size_t max_length = 128;
    for (size_t alloc_length = 2; alloc_length < max_length; alloc_length++) {
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++, ch++) {
                fill_wcharstring(str_offset, length_expected);
                ASSERT_NE(wmemset(str_offset, ch, length_expected), nullptr);
                if (ch == L'z') {
                    ch = L'a';
                }
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocwmemsetrandomsize) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset;
    wchar_t ch = L'a';
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_wcharstring(str_offset, alloc_length);
        ASSERT_NE(wmemset(str_offset, ch, alloc_length), nullptr);
        if (ch == L'z') {
            ch = L'a';
        }
        free(buff);
    }
}

TEST(STRING, mallocwmemcmp) {
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
                wmemcpy(str, str_offset, length_expected + 1);
                ASSERT_EQ(wmemcmp(str, str_offset, length_expected + 1), 0);
                free(str);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocwmemcmprandomsize) {
    srand(SRAND_SEED);
    wchar_t *buff, *str_offset, *str;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_wcharstring(str_offset, alloc_length);
        str = (wchar_t *)malloc(sizeof(wchar_t) * (alloc_length + 1));
        wmemcpy(str, str_offset, alloc_length + 1);
        ASSERT_EQ(wmemcmp(str, str_offset, alloc_length + 1), 0);
        free(str);
        free(buff);
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
