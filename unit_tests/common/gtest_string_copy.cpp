// model: *
#include <limits.h>
#include <string.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <gtest/gtest.h>
using namespace std;  // NOLINT

#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wunused-parameter"
#pragma GCC diagnostic error "-Wunused-variable"

#define MAX_STR 4096

// #define dbgprint(...) fprintf(stderr, __VA_ARGS__)
#ifndef dbgprint
#define dbgprint(...)
#endif

#define SRAND_SEED 0

static char s_buf[MAX_STR];

#define fill_buf(b, e)                                                         \
    for (int i = 0; i < e; ++i) {                                              \
        *b = 'a';                                                              \
    }

void fill_string(char *str, size_t size) {
    if (size != 0) {
        for (size_t i = 0; i < size; i++) {
            str[i] = '!' + i % ('~' - '!');  // pick any ascii between ! and ~
        }
    }
    str[size - 1] = '\0';
}

TEST(StringCopy, strncpy_simple) {
    const char *str = "Hello World\n";
    char *m_buf = (char *)malloc(MAX_STR * sizeof(char));
    fill_buf(m_buf, MAX_STR);

    char l_buf[MAX_STR]{0};

    strncpy(s_buf, str, MAX_STR);
    strncpy(l_buf, str, MAX_STR);
    strncpy(m_buf, str, MAX_STR);
    ASSERT_STREQ(str, s_buf);
    ASSERT_STREQ(str, l_buf);
    ASSERT_STREQ(str, m_buf);

    strncpy(l_buf, s_buf, MAX_STR);
    strncpy(m_buf, s_buf, MAX_STR);
    ASSERT_STREQ(str, s_buf);
    ASSERT_STREQ(str, l_buf);
    ASSERT_STREQ(str, m_buf);

    strncpy(s_buf, l_buf, MAX_STR);
    strncpy(m_buf, l_buf, MAX_STR);
    ASSERT_STREQ(str, s_buf);
    ASSERT_STREQ(str, l_buf);
    ASSERT_STREQ(str, m_buf);

    strncpy(s_buf, m_buf, MAX_STR);
    strncpy(l_buf, m_buf, MAX_STR);
    ASSERT_STREQ(str, s_buf);
    ASSERT_STREQ(str, l_buf);
    ASSERT_STREQ(str, m_buf);
}

TEST(StringCopy, strncpy_std_string_c_str) {
    std::string str = "Hello World\n";
    char *m_buf = (char *)malloc(MAX_STR * sizeof(char));
    fill_buf(m_buf, MAX_STR);

    char l_buf[MAX_STR]{0};

    strncpy(s_buf, str.c_str(), MAX_STR);
    strncpy(l_buf, str.c_str(), MAX_STR);
    strncpy(m_buf, str.c_str(), MAX_STR);
    ASSERT_STREQ(str.c_str(), s_buf);
    ASSERT_STREQ(str.c_str(), l_buf);
    ASSERT_STREQ(str.c_str(), m_buf);

    strncpy(l_buf, s_buf, MAX_STR);
    strncpy(m_buf, s_buf, MAX_STR);
    ASSERT_STREQ(str.c_str(), s_buf);
    ASSERT_STREQ(str.c_str(), l_buf);
    ASSERT_STREQ(str.c_str(), m_buf);

    strncpy(s_buf, l_buf, MAX_STR);
    strncpy(m_buf, l_buf, MAX_STR);
    ASSERT_STREQ(str.c_str(), s_buf);
    ASSERT_STREQ(str.c_str(), l_buf);
    ASSERT_STREQ(str.c_str(), m_buf);

    strncpy(s_buf, m_buf, MAX_STR);
    strncpy(l_buf, m_buf, MAX_STR);
    ASSERT_STREQ(str.c_str(), s_buf);
    ASSERT_STREQ(str.c_str(), l_buf);
    ASSERT_STREQ(str.c_str(), m_buf);
}

TEST(StringCopy, stack_std_string_copy_to_stack) {
    std::string str = "Hello World\n";
    char l_buf[MAX_STR]{0};
    fill_buf(l_buf, MAX_STR);

    dbgprint("c_str is at %016lx\n", (uintptr_t)str.c_str());
    dbgprint("l_buf is at %016lx\n", (uintptr_t)l_buf);

    size_t len = str.copy(l_buf, MAX_STR);
    l_buf[len] = '\0';
    ASSERT_STREQ(str.c_str(), l_buf);
}

TEST(StringCopy, stack_std_string_copy_to_static) {
    std::string str = "Hello World\n";

    dbgprint("c_str is at %016lx\n", (uintptr_t)str.c_str());
    dbgprint("s_buf is at %016lx\n", (uintptr_t)s_buf);

    size_t len = str.copy(s_buf, MAX_STR);
    s_buf[len] = '\0';
    ASSERT_STREQ(str.c_str(), s_buf);
}

TEST(StringCopy, stack_std_string_copy_to_heap) {
    std::string str = "Hello World\n";
    char *m_buf = (char *)malloc((MAX_STR) * sizeof(char));

    dbgprint("c_str is at %016lx\n", (uintptr_t)str.c_str());
    dbgprint("m_buf is at %016lx\n", (uintptr_t)m_buf);

    // Make sure we properly set null terminator
    memset(m_buf, 'a', MAX_STR);

    // str.copy doesn't set C string terminator
    size_t len = str.copy(m_buf, MAX_STR);
    m_buf[len] = '\0';

    ASSERT_STREQ(str.c_str(), m_buf);
}

TEST(StringCopy, heap_std_string_copy_to_heap) {
    std::string *str = new std::string("Hello World\n");
    char *m_buf = (char *)malloc(MAX_STR * sizeof(char));

    dbgprint("c_str is at %016lx\n", (uintptr_t)str->c_str());
    dbgprint("m_buf is at %016lx\n", (uintptr_t)m_buf);

    size_t len = str->copy(m_buf, MAX_STR);
    m_buf[len] = '\0';
    ASSERT_STREQ(str->c_str(), m_buf);
}

TEST(StringCopy, heap_std_string_copy_to_static) {
    std::string *str = new std::string("Hello World\n");

    dbgprint("c_str is at %016lx\n", (uintptr_t)str->c_str());
    dbgprint("s_buf is at %016lx\n", (uintptr_t)s_buf);

    size_t len = str->copy(s_buf, MAX_STR);
    s_buf[len] = '\0';
    ASSERT_STREQ(str->c_str(), s_buf);
}

TEST(StringCopy, heap_std_string_copy_to_stack) {
    std::string *str = new std::string("Hello World\n");
    char l_buf[MAX_STR]{0};

    dbgprint("c_str is at %016lx\n", (uintptr_t)str->c_str());
    dbgprint("l_buf is at %016lx\n", (uintptr_t)l_buf);

    size_t len = str->copy(l_buf, MAX_STR);
    l_buf[len] = '\0';
    ASSERT_STREQ(str->c_str(), l_buf);
}

TEST(StringCopy, static_std_string_copy) {
    size_t len;
    static std::string str = "Hello World\n";
    char *m_buf = (char *)malloc(MAX_STR * sizeof(char));
    char l_buf[MAX_STR]{0};

    dbgprint("c_str is at %016lx\n", (uintptr_t)str.c_str());
    dbgprint("s_buf is at %016lx\n", (uintptr_t)s_buf);
    dbgprint("l_buf is at %016lx\n", (uintptr_t)l_buf);
    dbgprint("m_buf is at %016lx\n", (uintptr_t)m_buf);

    len = str.copy(s_buf, MAX_STR);
    s_buf[len] = '\0';
    len = str.copy(l_buf, MAX_STR);
    l_buf[len] = '\0';
    len = str.copy(m_buf, MAX_STR);
    m_buf[len] = '\0';
    ASSERT_STREQ(str.c_str(), s_buf);
    ASSERT_STREQ(str.c_str(), l_buf);
    ASSERT_STREQ(str.c_str(), m_buf);
}

TEST(STRING, mallocstrcpy) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = 32;
    for (size_t alloc_length = 1; alloc_length < max_length; alloc_length++) {
        buff = (char *)malloc(sizeof(char) * (alloc_length + 17));
        for (int offset = 0; offset < 16; offset++) {
            str_offset = buff + offset;
            for (size_t length_expected = 1; length_expected < alloc_length;
                 length_expected++) {
                ASSERT_LE(length_expected + offset, alloc_length + 17);
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

TEST(STRING, stackstrcpy) {
    srand(SRAND_SEED);
    char *buff, *str_offset, *str;
    size_t max_length = 32;
    char arr_buff[max_length];
    char arr_str[max_length];

    for (size_t size = 1; size < max_length; size++) {
        for (int offset = 0; offset < 16; offset++) {
            for (size_t len = 1; (len + offset) < max_length; ++len) {
                ASSERT_LE(len + offset, max_length);

                buff = arr_buff;
                str = arr_str;
                str_offset = buff + offset;

                fill_string(str_offset, len);
                strcpy(str, str_offset);
                EXPECT_STREQ(str, str_offset);
            }
        }
    }
}

void stackstrcpy_dyn_do(size_t size, int offset, size_t len) {
    ASSERT_LE(len + offset, size);
    char src[size];
    char dst[size];

    char *bsrc = &src[offset];
    char *bdst = &dst[0];

    fill_string(bdst, len);
    strcpy(bdst, bsrc);
    EXPECT_STREQ(bsrc, bdst);
}

TEST(STRING, stackstrcpy_dyn) {
    srand(SRAND_SEED);
    size_t max_length = 32;

    for (size_t size = 1; size < max_length; size++) {
        for (int offset = 0; offset < 16; offset++) {
            for (size_t len = 1; (len + offset) < size; ++len) {
                stackstrcpy_dyn_do(size, offset, len);
            }
        }
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#pragma GCC diagnostic pop
