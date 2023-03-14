// model: *
#include <limits.h>
#include <string.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <gtest/gtest.h>
using namespace std;

#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wunused-parameter"
#pragma GCC diagnostic error "-Wunused-variable"

// #define dbgprint(...) fprintf(stderr, __VA_ARGS__)
#ifndef dbgprint
#define dbgprint(...)
#endif

class RealPathTest : public ::testing::Test {
 private:
    static constexpr const char *fn_base = "realpath_test";
    std::string filename_str;

 protected:
    const char *filename;

    RealPathTest() {
        static char fn_counter = 97;
        filename_str.assign(fn_base);
        filename_str.append(std::string(&fn_counter));
        ++fn_counter;
        filename = filename_str.c_str();
    }

    const std::string &getFilename_str() { return filename_str; }

    virtual void SetUp() {
        ofstream f(filename);
        f << "First line.\n";
        f << "Second line.\n";
    }

    virtual void TearDown() {
        ifstream f(filename);
        if (f.good()) {
            f.close();
            std::remove(filename);
        }
    }
};

static char s_name[PATH_MAX];
#define test_with_buf(b, r)                                                    \
    do {                                                                       \
        /* set up and sanity check */                                          \
        char l_name[PATH_MAX];                                                 \
        getFilename_str().copy(l_name, PATH_MAX);                              \
        getFilename_str().copy(s_name, PATH_MAX);                              \
                                                                               \
        /* workaround for .copy() bug */                                       \
        l_name[getFilename_str().length()] = '\0';                             \
        s_name[getFilename_str().length()] = '\0';                             \
                                                                               \
        /* check we got what we expect */                                      \
        ASSERT_EQ(nullptr, r);                                                 \
        ASSERT_STREQ(filename, l_name);                                        \
        ASSERT_STREQ(filename, s_name);                                        \
                                                                               \
        dbgprint("filename: %016lx\n", (uintptr_t)filename);                   \
        dbgprint("l_name:   %016lx\n", (uintptr_t)l_name);                     \
        dbgprint("s_name:   %016lx\n", (uintptr_t)s_name);                     \
        dbgprint("b:        %016lx\n", (uintptr_t)b);                          \
        dbgprint("r:        %016lx\n", (uintptr_t)r);                          \
                                                                               \
        /* Test with name on heap */                                           \
        r = realpath(filename, b);                                             \
        dbgprint("heap:     %016lx %s\n", (uintptr_t)r, r);                    \
        ASSERT_NE(nullptr, r);                                                 \
        if (b != NULL) {                                                       \
            ASSERT_EQ(b, r);                                                   \
        }                                                                      \
                                                                               \
        /* Test with name on stack */                                          \
        r = realpath(l_name, b);                                               \
        dbgprint("stack:    %016lx %s\n", (uintptr_t)r, r);                    \
        ASSERT_NE(nullptr, r);                                                 \
        if (b != NULL) {                                                       \
            ASSERT_EQ(b, r);                                                   \
        }                                                                      \
                                                                               \
        /* Test with name in static global */                                  \
        r = realpath(s_name, b);                                               \
        dbgprint("static:   %016lx %s\n", (uintptr_t)r, r);                    \
        ASSERT_NE(nullptr, r);                                                 \
        if (b != NULL)                                                         \
            ASSERT_EQ(b, r);                                                   \
    } while (0);

static char s_buf[PATH_MAX];
TEST_F(RealPathTest, static_buf) {
    char *r = NULL;
    test_with_buf(s_buf, r);
}

TEST_F(RealPathTest, heap_buf) {
    char *r = NULL;
    char *buf = (char *)malloc(PATH_MAX);
    ASSERT_NE(buf, nullptr);
    test_with_buf(buf, r);
    free(buf);
}

TEST_F(RealPathTest, stack_buf) {
    char *r = NULL;
    char buf[PATH_MAX];
    test_with_buf(buf, r);
}

TEST_F(RealPathTest, no_buf) {
    char *r = NULL;
    char *buf = NULL;
    test_with_buf(buf, r);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#pragma GCC diagnostic pop
