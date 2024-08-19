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

TEST(STRING, mallocmemchr) {
    srand(SRAND_SEED);
    char *buff, *str_offset;
    size_t max_length = 128;
    char ch = '!';
    for (size_t alloc_length = 5; alloc_length < max_length;
         alloc_length++, ch = '!') {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++, ch = '!') {
            str_offset = buff + offset;
            for (size_t length_expected = 3; length_expected < alloc_length;
                 length_expected++, ch++) {
                fill_string(str_offset, length_expected);
                ASSERT_NE(memchr(str_offset, ch, length_expected), nullptr);
                if (ch == '|') {
                    ch = '!';
                }
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocmemchrrandomsize) {
    srand(SRAND_SEED);
    char *buff, *str_offset;
    char ch = '!';
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++, ch++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (char *)malloc(sizeof(char) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_string(str_offset, alloc_length);
        ASSERT_NE(memchr(str_offset, ch, alloc_length), nullptr);
        if (ch == '|') {
            ch = '!';
        }
        free(buff);
    }
}

TEST(STRING, mallocmemchr1) {
    srand(SRAND_SEED);
    char *buff, *str_offset;
    char ch = ' ';
    size_t max_length = 128;
    for (size_t alloc_length = 3; alloc_length < max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 2; length_expected < alloc_length;
                 length_expected++) {
                fill_string(str_offset, length_expected);
                ASSERT_EQ(memchr(str_offset, ch, length_expected), nullptr);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocmemchr1randomsize) {
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
        ASSERT_EQ(memchr(str_offset, ch, alloc_length), nullptr);
        free(buff);
    }
}

TEST(STRING, mallocmemrchr) {
    srand(SRAND_SEED);
    char *buff, *str_offset;
    char ch = '!';
    size_t max_length = 128;
    for (size_t alloc_length = 5; alloc_length < max_length;
         alloc_length++, ch = '!') {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++, ch = '!') {
            str_offset = buff + offset;
            for (size_t length_expected = 3; length_expected < alloc_length;
                 length_expected++, ch++) {
                fill_string(str_offset, length_expected);
                ASSERT_NE(memrchr(str_offset, ch, length_expected), nullptr);
                if (ch == '|') {
                    ch = '!';
                }
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocmemrchrrandomsize) {
    srand(SRAND_SEED);
    char *buff, *str_offset;
    char ch = '!';
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++, ch++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (char *)malloc(sizeof(char) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_string(str_offset, alloc_length);
        ASSERT_NE(memrchr(str_offset, ch, alloc_length), nullptr);
        if (ch == '|') {
            ch = '!';
        }
        free(buff);
    }
}

TEST(STRING, mallocmemrchr1) {
    srand(SRAND_SEED);
    char *buff, *str_offset;
    char ch = ' ';
    size_t max_length = 128;
    for (size_t alloc_length = 3; alloc_length < max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 2; length_expected < alloc_length;
                 length_expected++) {
                fill_string(str_offset, length_expected);
                ASSERT_EQ(memrchr(str_offset, ch, length_expected), nullptr);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocmemrchr1randomsize) {
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
        ASSERT_EQ(memrchr(str_offset, ch, alloc_length), nullptr);
        free(buff);
    }
}

TEST(STRING, mallocstrfry) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = 128;
    for (size_t alloc_length = 12; alloc_length < max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 10; length_expected < alloc_length;
                 length_expected++) {
                fill_randomstring(str_offset, length_expected);
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                strcpy(str, str_offset);
                char *returnval;
                returnval = strfry(str_offset);
                ASSERT_TRUE(
                        (strcmp(str, returnval) != 0) ||
                        // Re-randomize, in case we get same string by chance
                        (strcmp(str, strfry(str_offset)) != 0));
                free(str);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocstrfryrandomsize) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (char *)malloc(sizeof(char) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_randomstring(str_offset, alloc_length);
        str = (char *)malloc(sizeof(char) * (alloc_length + 1));
        strcpy(str, str_offset);
        char *returnval;
        returnval = strfry(str_offset);
        ASSERT_TRUE((strcmp(str, returnval) != 0) ||
                    // Re-randomize, in case we get same string by chance
                    (strcmp(str, strfry(str_offset)) != 0));
        free(str);
        free(buff);
    }
}

TEST(STRING, mallocstrcoll) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = 128;
    for (size_t alloc_length = 5; alloc_length < max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 3; length_expected < alloc_length;
                 length_expected++) {
                fill_string(str_offset, length_expected);
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                strcpy(str, str_offset);
                ASSERT_EQ(strcoll(str, str_offset), 0);
                free(str);
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                fill_randomstring(str, length_expected);
                ASSERT_NE(strcoll(str, str_offset), 0);
                free(str);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocstrcollrandomsize) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (char *)malloc(sizeof(char) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_string(str_offset, alloc_length);
        str = (char *)malloc(sizeof(char) * (alloc_length + 1));
        strcpy(str, str_offset);
        ASSERT_EQ(strcoll(str, str_offset), 0);
        free(str);
        str = (char *)malloc(sizeof(char) * (alloc_length + 1));
        fill_randomstring(str, alloc_length);
        ASSERT_NE(strcoll(str, str_offset), 0);
        free(str);
        free(buff);
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
