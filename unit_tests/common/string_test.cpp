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

#define MAGIC(n)                                                               \
    do {                                                                       \
        int simics_magic_instr_dummy;                                          \
        __asm__ __volatile__("cpuid"                                           \
                             : "=a"(simics_magic_instr_dummy)                  \
                             : "a"(0x4711 | ((unsigned)(n) << 16))             \
                             : "ecx", "edx", "ebx");                           \
    } while (0)

#define SRAND_SEED 0

// Helper function to init ref data buffer with random data
/*
static void init_reference_data_random(uint8_t * buffer, int num) {
    srand(SRAND_SEED);
    for (int i = 0; i<num; i++) {
        buffer[i] = (uint8_t) rand();
    }
}
*/

static void init_reference_data_sequential(uint8_t *buffer, int num) {
    uint8_t sequence = 0x0;
    for (int i = 0; i < num; i++) {
        buffer[i] = sequence++;
    }
}

/*
static void init_reference_data_0x11(uint8_t * buffer, int num) {
    for (int i = 0; i<num; i++) {
        buffer[i] = 0x11;
    }
}
*/
/*
TEST(MEMCPY, Case4849) {
    int max_size = 1 << 20;
    int ref_size = max_size;
    uint8_t ref_data[ref_size];
    init_reference_data_0x11(ref_data, ref_size);
    int bytes = 4849;
    uint8_t* p = (uint8_t*) calloc (1, bytes);
    memcpy((void*) p, (void*) ref_data, bytes);
    for (int i = 0; i < bytes; i++) {
        if (ref_data[i] != p[i]){
            MAGIC(0);
            printf("ERROR: Mismatch detected:\n");
            printf("num bytes=%d, i=%d, p=0x%016lx\n", bytes, i, (uint64_t) p);
            ASSERT_EQ(ref_data[i], p[i]);
        }
    }
    free(p);
}
*/

size_t BLI_snprintf(char *dst, size_t maxncpy, const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    size_t n = vsnprintf(dst, maxncpy, format, arg);
    va_end(arg);
    return n;
}
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

TEST(MEM, memcmp) {
    uint8_t byte1 = 0;
    uint8_t byte2 = 0;
    uint8_t buf1[] = {0, 0, 0, 0, byte1};
    uint8_t buf2[] = {0, 0, 0, 0, byte2};
    do {
        do {
            buf1[4] = byte1;
            buf2[4] = byte2;
            int result = memcmp(buf1, buf2, 5);
            int diff = (int)(byte1 - byte2);
            if (diff == 0) {
                ASSERT_EQ(result, 0);
            } else if (diff < 0) {
                ASSERT_LT(result, 0);
            } else {
                ASSERT_GT(result, 0);
            }
            byte1++;
        } while (byte1 != 0);
        byte2++;
    } while (byte2 != 0);
}

TEST(WCSTOMBS, GetLength) {
    char str[] = "t5.xml";
    wchar_t *w_buf = (wchar_t *)malloc(32);
    memset(w_buf, 0xab, 32);
    int length = strlen(str);
    ASSERT_EQ(length, 6);
    for (int i = 0; i < length; i++) {
        w_buf[i] = (wchar_t)str[i];
    }
    char *buf = new char[length + 1];
    memset(buf, 0xab, length + 1);
    int num = wcstombs(buf, w_buf, length);
    ASSERT_EQ(num, 6);
    buf[num] = '\0';
    int buf_length = strlen(buf);
    ASSERT_EQ(buf_length, 6);
    ASSERT_EQ(strcmp(buf, str), 0);
}

TEST(STRING, strlen) {
    const char str[] = "abcdef0123";
    int length_expected = 10;
    int length = strlen(str);
    ASSERT_EQ(length, length_expected);
}

TEST(STRING, strnlen1) {
    const char str[] = "abcdef0123";
    int length_expected = 5;
    int length = strnlen(str, 5);
    ASSERT_EQ(length, length_expected);
}

TEST(STRING, strcpy) {
    const char str1[] = "abcdefgh01235678";
    int length = strlen(str1) + 1;
    char *str2;
    str2 = (char *)malloc(sizeof(char) * length);
    strcpy(str2, str1);
    ASSERT_STREQ(str2, str1);
    free(str2);
}

TEST(STRING, strncpy) {
    const char str1[] = "abcdefABCDEF012356";
    int length = strlen(str1) + 1;
    const char str_expected[] = "abcdefABCD";
    int expstr_length = strlen(str_expected);
    char *str2;
    str2 = (char *)malloc(sizeof(char) * length);
    strncpy(str2, str1, expstr_length);
    str2[expstr_length] = '\0';
    ASSERT_STREQ(str2, str_expected);
    free(str2);
}

TEST(STRING, mallocstrcpy) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = 128;
    for (size_t alloc_length = 1; alloc_length < max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++) {
                fill_string(str_offset, length_expected);
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                strcpy(str, str_offset);
                ASSERT_STREQ(str, str_offset);
                free(str);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocstrcpyrandomsize) {
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
        ASSERT_STREQ(str, str_offset);
        free(str);
        free(buff);
    }
}

TEST(STRING, mallocstrncpy) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = 128;
    for (size_t alloc_length = 2; alloc_length < max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++) {
                fill_string(str_offset, length_expected);
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                strncpy(str, str_offset, length_expected);
                str[length_expected] = '\0';
                ASSERT_STREQ(str, str_offset);
                free(str);
                str = (char *)malloc(sizeof(char) * (length_expected + 1));
                strncpy(str, str_offset, length_expected + 1);
                ASSERT_STREQ(str, str_offset);
                free(str);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocstrncpyrandomsize) {
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
        strncpy(str, str_offset, alloc_length);
        str[alloc_length] = '\0';
        ASSERT_STREQ(str, str_offset);
        free(str);
        str = (char *)malloc(sizeof(char) * (alloc_length + 1));
        strncpy(str, str_offset, alloc_length + 1);
        ASSERT_STREQ(str, str_offset);
        free(str);
        free(buff);
    }
}

TEST(STRING, strcat) {
    const char str1[] = "abcdefghABCDEFGH";
    int length1 = strlen(str1);
    const char str2[] = "0123456789";
    int length2 = strlen(str2);
    const char str_expected[] = "abcdefghABCDEFGH0123456789";
    int expstr_length = strlen(str_expected);
    char *str3;
    str3 = (char *)malloc(sizeof(char) * (expstr_length * 2));
    strcpy(str3, str1);
    strcat(str3, str2);
    ASSERT_STREQ(str3, str_expected);
    free(str3);
}

TEST(STRING, mallocstrcat) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str1, *str2;
#ifdef KEEP_SLOW  // Moved to string_test_slow.cpp
    size_t max_length = 128;
#else
    size_t max_length = 64;
#endif
    for (size_t alloc_length = 2; alloc_length < max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++) {
                fill_string(str_offset, length_expected);
                str1 = (char *)malloc(sizeof(char) * (length_expected + 1));
                fill_string(str1, length_expected);
                str2 = (char *)malloc(2 * sizeof(char) * (length_expected + 1));
                strcpy(str2, str1);
                strcat(str2, str_offset);
                ASSERT_STRNE(strstr(str2, str_offset), nullptr);
                ASSERT_STRNE(strstr(str2, str2), nullptr);
                free(str1);
                free(str2);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocstrcatrandomsize) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str1, *str2;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (char *)malloc(sizeof(char) * alloc_length + offset + 1);
        str_offset = buff + offset;
        fill_string(str_offset, alloc_length);
        str1 = (char *)malloc(sizeof(char) * (alloc_length + 1));
        fill_string(str1, alloc_length);
        str2 = (char *)malloc(2 * sizeof(char) * (alloc_length + 1));
        strcpy(str2, str1);
        strcat(str2, str_offset);
        ASSERT_STRNE(strstr(str2, str_offset), nullptr);
        ASSERT_STRNE(strstr(str2, str1), nullptr);
        free(str1);
        free(str2);
        free(buff);
    }
}

TEST(STRING, mallocstrncat) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str1, *str2;
#ifdef KEEP_SLOW  // Moved to string_test_slow.cpp
    size_t max_length = 128;
#else
    size_t max_length = 64;
#endif
    for (size_t alloc_length = 2; alloc_length < max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++) {
                fill_string(str_offset, length_expected);
                str1 = (char *)malloc(sizeof(char) * (length_expected + 1));
                fill_string(str1, length_expected);
                str2 = (char *)malloc(2 * sizeof(char) * (length_expected + 1));
                strcpy(str2, str1);
                strncat(str2, str_offset, length_expected);
                ASSERT_STRNE(strstr(str2, str_offset), nullptr);
                ASSERT_STRNE(strstr(str2, str1), nullptr);
                free(str1);
                free(str2);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocstrncatrandomsize) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str1, *str2;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (char *)malloc(sizeof(char) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_string(str_offset, alloc_length);
        str1 = (char *)malloc(sizeof(char) * (alloc_length + 1));
        fill_string(str1, alloc_length);
        str2 = (char *)malloc(2 * sizeof(char) * (alloc_length + 1));
        strcpy(str2, str1);
        strncat(str2, str_offset, alloc_length);
        ASSERT_STRNE(strstr(str2, str_offset), nullptr);
        ASSERT_STRNE(strstr(str2, str1), nullptr);
        free(str1);
        free(str2);
        free(buff);
    }
}

TEST(STRING, mallocstrlen) {
    srand(SRAND_SEED);
    char *buff, *str_offset;
    size_t max_length = 128;
    for (size_t alloc_length = 1; alloc_length < max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 0; length_expected < alloc_length;
                 length_expected++) {
                fill_string(str_offset, length_expected);
                size_t length = strlen(str_offset);
                if (length != length_expected) {
                    printf("alloc_length    = %ld\n", alloc_length);
                    printf("length_expected = %ld\n", length_expected);
                }
                ASSERT_EQ(length, length_expected);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocstrlenrandomsize) {
    srand(SRAND_SEED);
    char *buff, *str_offset;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (char *)malloc(sizeof(char) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_string(str_offset, alloc_length);
        size_t length = strlen(str_offset);
        if (length != alloc_length) {
            printf("alloc_length    = %ld\n", alloc_length);
        }
        ASSERT_EQ(length, alloc_length);
        free(buff);
    }
}

TEST(STRING, mallocstrnlen) {
    srand(SRAND_SEED);
    char *buff, *str_offset;
    size_t max_length = 128;
    for (size_t alloc_length = 5; alloc_length < max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 3; length_expected < alloc_length;
                 length_expected++) {
                fill_string(str_offset, length_expected);
                // validating all 3 cases when length is less/equal/larger than
                // the actual string length
                ASSERT_EQ(strnlen(str_offset, (length_expected - 2)),
                          length_expected - 2);
                ASSERT_EQ(strnlen(str_offset, length_expected),
                          length_expected);
                ASSERT_EQ(strnlen(str_offset, (length_expected + 5)),
                          length_expected);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocstrnlenrandomsize) {
    srand(SRAND_SEED);
    char *buff, *str_offset;
    size_t max_length = 4096 * 4;
    size_t alloc_length;
    for (size_t i = 1; i <= 100; i++) {
        alloc_length = rand() % max_length + 17;
        int offset = rand() % 16;
        buff = (char *)malloc(sizeof(char) * (alloc_length + offset + 1));
        str_offset = buff + offset;
        fill_string(str_offset, alloc_length);
        // validating all 3 cases when length is less/equal/larger than the
        // actual string length
        ASSERT_EQ(strnlen(str_offset, (alloc_length - 2)), alloc_length - 2);
        ASSERT_EQ(strnlen(str_offset, alloc_length), alloc_length);
        ASSERT_EQ(strnlen(str_offset, (alloc_length + 5)), alloc_length);
        free(buff);
    }
}

TEST(FORMAT, blender_vsnprintf) {
    const char *path = "cube_####";
    const char *out_str_expected = "cube_0001";
    int ch_sta = 5;
    int ch_end = 9;
    int frame = 1;
    char out_str[1024];
    BLI_snprintf(out_str, sizeof(out_str), "%.*s%.*d%s", ch_sta, path,
                 ch_end - ch_sta, frame, path + ch_end);
    ASSERT_STREQ(out_str, out_str_expected);
}

TEST(MEMCPY, CopyNBytesFromRefToBuffer) {
#ifdef KEEP_SLOW  // Moved to string_test_slow.cpp
    int max_size = 1 << 16;
    int max_bytes = 1 << 13
#else
    int max_size = 1 << 13;
    int max_bytes = 1 << 9;
#endif
                    int ref_size = max_size;
    uint8_t ref_data[ref_size];
    init_reference_data_sequential(ref_data, ref_size);
    for (int bytes = 0; bytes < max_bytes; bytes++) {
        uint8_t *p = (uint8_t *)calloc(1, bytes);
        /*
        if (bytes==4856){
            printf("num bytes=%d, p=0x%016lx\n", bytes, (uint64_t) p);
            printf("&p[4840] = 0x%016lx\n",  (uint64_t) &p[4840]);
            printf("MAGIC BREAKPOINT before memcpy\n");
            MAGIC(0);
            memcpy((void*) p, (void*) ref_data, bytes);
            printf("MAGIC BREAKPOINT after memcpy\n");
            MAGIC(0);
        } else {
            memcpy((void*) p, (void*) ref_data, bytes);
        }
        */
        memcpy((void *)p, (void *)ref_data, bytes);
        for (int i = 0; i < bytes; i++) {
            if (ref_data[i] != p[i]) {
                printf("ERROR: Mismatch detected:\n");
                printf("num bytes=%d, i=%d, p=0x%016lx\n", bytes, i,
                       (uint64_t)p);
                printf("MAGIC BREAKPOINT after mismatch\n");
                MAGIC(0);
                ASSERT_EQ(ref_data[i], p[i]);
            }
        }
        free(p);
    }
}

TEST(STRING, mallocstrchr) {
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
                ASSERT_NE(strchr(str_offset, ch), nullptr);
                if (ch == '|') {
                    ch = '!';
                }
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocstrchrrandomsize) {
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
        ASSERT_NE(strchr(str_offset, ch), nullptr);
        if (ch == '|') {
            ch = '!';
        }
        free(buff);
    }
}

TEST(STRING, mallocstrchr1) {
    srand(SRAND_SEED);
    char *buff, *str_offset;
    char ch = ' ';
    size_t max_length = 128;
    for (size_t alloc_length = 10; alloc_length <= max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 5; length_expected < alloc_length;
                 length_expected++) {
                fill_string(str_offset, length_expected);
                ASSERT_EQ(strchr(str_offset, ch), nullptr);
            }
        }
        free(buff);
    }
}

TEST(STRING, mallocstrchr1randomsize) {
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
        ASSERT_EQ(strchr(str_offset, ch), nullptr);
        free(buff);
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
