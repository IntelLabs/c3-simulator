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

TEST(STRING, strchr) {
    char *str;
    char ch = '*';
    str = (char *)malloc(20 * sizeof(char));
    fill_string(str, 20);
    ASSERT_NE(strchr(str, ch), nullptr);
    free(str);
}

TEST(STRING, mallocstrrchr) {
    srand(SRAND_SEED);
    char *buff, *str_offset;
    char ch = '!';
    size_t max_length = 128;
    for (size_t alloc_length = 3; alloc_length <= max_length;
         alloc_length++, ch = '!') {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++, ch = '!') {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++, ch++) {
                fill_string(str_offset, length_expected + 1);
                ASSERT_NE(strrchr(str_offset, ch), nullptr);
                if (ch == '|') {
                    ch = '!';
                }
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocstrrchrrandomsize) {
    srand(SRAND_SEED);
    char *buff, *str_offset;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    char ch = '!';
    for (size_t i = 1; i <= 100; i++, ch++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (char *)malloc(sizeof(char) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_string(str_offset, alloc_length);
        ASSERT_NE(strrchr(str_offset, ch), nullptr);
        if (ch == '|') {
            ch = '!';
        }
        free(buff);
    }
}

TEST(STRING, mallocstrrchr1) {
    srand(SRAND_SEED);
    char *buff, *str_offset;
    char ch = ' ';
    size_t max_length = 128;
    for (size_t alloc_length = 3; alloc_length <= max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++) {
                fill_string(str_offset, length_expected);
                ASSERT_EQ(strrchr(str_offset, ch), nullptr);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocstrrchr1randomsize) {
    srand(SRAND_SEED);
    char *buff, *str_offset;
    char ch = ' ';
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (char *)malloc(sizeof(char) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_string(str_offset, alloc_length);
        ASSERT_EQ(strrchr(str_offset, ch), nullptr);
        free(buff);
    }
}

TEST(STRING, mallocstrchrnul) {
    srand(SRAND_SEED);
    char *buff, *str_offset;
    char ch = '!';
    size_t max_length = 128;
    for (size_t alloc_length = 3; alloc_length <= max_length;
         alloc_length++, ch = '!') {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++, ch = '!') {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++, ch++) {
                fill_string(str_offset, length_expected);
                ASSERT_NE(strchrnul(str_offset, ch), nullptr);
                if (ch == '|') {
                    ch = '!';
                }
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocstrchrnulrandomsize) {
    srand(SRAND_SEED);
    char *buff, *str_offset;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    char ch = '!';
    for (size_t i = 1; i <= 100; i++, ch++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (char *)malloc(sizeof(char) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_string(str_offset, alloc_length);
        ASSERT_NE(strchrnul(str_offset, ch), nullptr);
        if (ch == '|') {
            ch = '!';
        }
        free(buff);
    }
}

TEST(STRING, mallocstrchrnul1) {
    srand(SRAND_SEED);
    char *buff, *str_offset;
    char ch = ' ';
    size_t max_length = 128;
    for (size_t alloc_length = 3; alloc_length <= max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++) {
                fill_string(str_offset, length_expected);
                char *ret = strchrnul(str_offset, ch);
                char val = *ret;
                ASSERT_EQ(val, '\0');
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocstrchrnul1randomsize) {
    srand(SRAND_SEED);
    char *buff, *str_offset;
    char ch = ' ';
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (char *)malloc(sizeof(char) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_string(str_offset, alloc_length);
        char *ret = strchrnul(str_offset, ch);
        char val = *ret;
        ASSERT_EQ(val, '\0');
        free(buff);
    }
}

TEST(STRING, mallocstrpbrk) {
    char *buff, *str_offset, *keys;
    size_t max_length = 128;
#ifdef KEEP_SLOW  // Moved to string_test1_slow.cpp
    const size_t max_offset = 16;
#else
    const size_t max_offset = 9;
#endif
    size_t length_expected;
    for (size_t alloc_length = 5; alloc_length <= max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (max_length + 17));
        for (int offset = 0; offset < max_offset; offset++) {
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

TEST(STRING, mallocstrpbrkrandomsize) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *keys;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (char *)malloc(sizeof(char) * (max_length + offset + 1));
        str_offset = buff + offset;
        fill_string(str_offset, max_length);
        keys = (char *)malloc(sizeof(char) * (alloc_length + 1));
        fill_randomstring(keys, alloc_length);
        ASSERT_NE(strpbrk(str_offset, keys), nullptr);
        free(keys);
        free(buff);
    }
}

TEST(STRING, mallocstrspn) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str, *str1;
#ifdef KEEP_SLOW  // Moved to string_test1_slow.cpp
    size_t max_length = 128;
#else
    size_t max_length = 64;
#endif
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

TEST(STRING, mallocstrspnrandomsize) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (char *)malloc(sizeof(char) * (alloc_length + offset));
        str_offset = buff + offset;
        str = (char *)malloc(sizeof(char) * (alloc_length));
        if (alloc_length >= 10) {
            fill_randomnumbers(str_offset, alloc_length);
            fill_numbers(str, alloc_length);
        } else {
            fill_string(str_offset, alloc_length);
            fill_string(str, alloc_length);
        }
        ASSERT_EQ(strspn(str_offset, str), (alloc_length - 1));
        free(str);
        fill_string(str_offset, alloc_length);
        str = (char *)malloc(sizeof(char) * (alloc_length + 1));
        fill_randomstring(str, alloc_length);
        ASSERT_EQ(strspn(str_offset, str), 0);
        free(str);
        free(buff);
    }
}

TEST(STRING, mallocstrcspn) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str, *str1;
#ifdef KEEP_SLOW  // Moved to string_test1_slow.cpp
    size_t max_length = 128;
#else
    size_t max_length = 64;
#endif
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

TEST(STRING, mallocstrcspnrandomsize) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (char *)malloc(sizeof(char) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_randomalphabets(str_offset, alloc_length);
        str = (char *)malloc(sizeof(char) * (alloc_length + 1));
        fill_randomnumbers(str, alloc_length + 1);
        ASSERT_EQ(strcspn(str_offset, str), alloc_length - 1);
        free(str);
        str = (char *)malloc(sizeof(char) * (alloc_length + 1));
        fill_string(str_offset, alloc_length);
        fill_string(str, alloc_length);
        ASSERT_EQ(strcspn(str_offset, str), 0);
        free(str);
        free(buff);
    }
}

TEST(STRING, mallocstpcpy) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = 128;
    for (size_t alloc_length = 1; alloc_length <= max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++) {
                fill_string(str_offset, length_expected);
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                char *ret = stpcpy(str, str_offset);
                char value = *ret;
                ASSERT_STREQ(str, str_offset);
                ASSERT_EQ(value, '\0');
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
