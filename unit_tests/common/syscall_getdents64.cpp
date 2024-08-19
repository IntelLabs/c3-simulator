// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: *
// need_kernel: yes
#include <dirent.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <gtest/gtest.h>
using namespace std;

// #define dbgprint(...) fprintf(stderr, __VA_ARGS__)
#ifndef dbgprint
#define dbgprint(...)
#endif

#define _PATH1 "/tmp"
#define BUFF_SIZE 1024

const char *g_path1 = _PATH1;
char g_buff_arr[BUFF_SIZE];
char *g_buff = (char *)&g_buff_arr;

class PathTest : public ::testing::Test {
 private:
    char *_h_path1;

 protected:
    const char *h_path1;
    DIR *d;
    void *h_buff;

    PathTest() {}

    virtual void SetUp() {
        _h_path1 = (char *)malloc(sizeof(_PATH1));
        strncpy(_h_path1, _PATH1, sizeof(_PATH1));
        h_path1 = _h_path1;

        h_buff = malloc(BUFF_SIZE);
    }

    virtual void TearDown() {
        free((void *)_h_path1);
        h_path1 = NULL;
        _h_path1 = NULL;

        free(h_buff);

        if (d != NULL) {
            closedir(d);
            d = NULL;
        }
    }

    void test_opendir(const char *dir) {
        dbgprint("%s: 0x%016lx -> %s\n", __func__, (uint64_t)dir, dir);

        d = opendir(dir);
        if (d == NULL) {
            perror("opendir");
        }
        dbgprint("%s: got 0x%016lx\n", __func__, (uint64_t)d);
        ASSERT_FALSE(d == NULL);
    }

    void test_getdents64(const char *dir, void *data, size_t data_size) {
        // Open directory
        d = opendir(dir);
        if (d == NULL) {
            perror("opendir");
        }
        ASSERT_FALSE(d == NULL);

        ssize_t bytes = syscall(SYS_getdents64, dirfd(d), data, data_size);
        if (bytes < 0) {
            perror("getdents64");
        }
        ASSERT_GE(bytes, 0);
    }
};

// Check opendir works as expected

TEST_F(PathTest, opendir_heap) { test_opendir(h_path1); }

TEST_F(PathTest, opendir_stack) {
    const char *l_path1 = _PATH1;
    test_opendir(l_path1);
}

TEST_F(PathTest, opendir_global) { test_opendir(g_path1); }

// Test getdents64 with results buffer on heap

TEST_F(PathTest, getdents64_heap_heap) {
    test_getdents64(h_path1, h_buff, BUFF_SIZE);
}

TEST_F(PathTest, getdents64_stack_heap) {
    const char *l_path1 = _PATH1;
    test_getdents64(l_path1, h_buff, BUFF_SIZE);
}

TEST_F(PathTest, getdents64_global_heap) {
    test_getdents64(g_path1, h_buff, BUFF_SIZE);
}

// Test getdents64 with results buffer on stack

TEST_F(PathTest, getdents64_heap_stack) {
    char arr[BUFF_SIZE];
    void *l_buff = &arr;
    test_getdents64(h_path1, l_buff, BUFF_SIZE);
}

TEST_F(PathTest, getdents64_stack_stack) {
    char arr[BUFF_SIZE];
    void *l_buff = &arr;
    const char *l_path1 = _PATH1;
    test_getdents64(l_path1, l_buff, BUFF_SIZE);
}

TEST_F(PathTest, getdents64_global_stack) {
    char arr[BUFF_SIZE];
    void *l_buff = &arr;
    test_getdents64(g_path1, l_buff, BUFF_SIZE);
}

// Test getdents64 with results buffer on data section

TEST_F(PathTest, getdents64_heap_global) {
    test_getdents64(h_path1, g_buff, BUFF_SIZE);
}

TEST_F(PathTest, getdents64_stack_global) {
    const char *l_path1 = _PATH1;
    test_getdents64(l_path1, g_buff, BUFF_SIZE);
}

TEST_F(PathTest, getdents64_global_global) {
    test_getdents64(g_path1, g_buff, BUFF_SIZE);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
