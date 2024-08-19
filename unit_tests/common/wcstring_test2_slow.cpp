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

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
