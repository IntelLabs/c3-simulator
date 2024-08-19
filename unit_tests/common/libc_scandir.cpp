// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: *
// xfail: cc
// need_kernel: yes
// FIXME: Fix scandir failure on heap
#include <dirent.h>
#include <stdlib.h>
#include <gtest/gtest.h>
using namespace std;

// #define dbgprint(...) fprintf(stderr, __VA_ARGS__)
#ifndef dbgprint
#define dbgprint(...)
#endif

// int scandir(const char *dirp, struct dirent ***namelist,
//               int (*filter)(const struct dirent *),
//               int (*compar)(const struct dirent **, const struct dirent **));

#define _PATH1 "/tmp"

const char *g_path1 = _PATH1;

class ScandirTest : public ::testing::Test {
 private:
    char *_h_path1;

 protected:
    const char *h_path1;

    ScandirTest() {}

    virtual void SetUp() {
        _h_path1 = (char *)malloc(sizeof(_PATH1));
        strncpy(_h_path1, _PATH1, sizeof(_PATH1));
        h_path1 = _h_path1;
    }

    virtual void TearDown() {
        free((void *)_h_path1);
        h_path1 = NULL;
        _h_path1 = NULL;
    }

    void test_scandir(const char *dir) {
        dbgprint("%s: 0x%016lx -> %s\n", __func__, (uint64_t)dir, dir);

        struct dirent **dirlist = NULL;
        int n = scandir(_PATH1, &dirlist, NULL, alphasort);
        if (n < 0) {
            perror("scandir");
        }
        ASSERT_FALSE(n < 0);
    }

    void test_opendir(const char *dir) {
        dbgprint("%s: 0x%016lx -> %s\n", __func__, (uint64_t)dir, dir);

        DIR *d = opendir(dir);
        dbgprint("%s: got 0x%016lx\n", __func__, (uint64_t)d);
        if (d == NULL) {
            perror("opendir");
        }
        ASSERT_FALSE(d == NULL);
        closedir(d);
    }
};

TEST_F(ScandirTest, opendir_global) { test_opendir(g_path1); }

TEST_F(ScandirTest, scandir_global) { test_scandir(g_path1); }

TEST_F(ScandirTest, opendir_heap) { test_opendir(h_path1); }

TEST_F(ScandirTest, scandir_heap) { test_scandir(h_path1); }

TEST_F(ScandirTest, opendir_stack) {
    const char *l_path1 = _PATH1;
    test_opendir(l_path1);
}

TEST_F(ScandirTest, scandir_stack) {
    const char *l_path1 = _PATH1;
    test_scandir(l_path1);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
