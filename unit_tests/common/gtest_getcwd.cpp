// Copyright 2016 Intel Corporation
// SPDX-License-Identifier: MIT
// model: *

#include <unistd.h>
#include <gtest/gtest.h>

#define BUFF_SIZE 4096
#define test_getcwd_common(buff1)                                              \
    do {                                                                       \
        getcwd(buff1, BUFF_SIZE);                                              \
        char *buff2 = get_current_dir_name();                                  \
        ASSERT_NE(nullptr, buff1);                                             \
        ASSERT_NE(nullptr, buff2);                                             \
        ASSERT_STREQ(buff1, buff2);                                            \
        free(buff2);                                                           \
    } while (0)

TEST(GETCWD, getcwd_stack) {
    char buff1[BUFF_SIZE];

    test_getcwd_common(buff1);
}

TEST(GETCWD, getcwd_heap) {
    char *buff1 = reinterpret_cast<char *>(malloc(BUFF_SIZE));

    test_getcwd_common(buff1);
}

TEST(GETCWD, getcwd_heap_nullptr) {
    char *buff1 = reinterpret_cast<char *>(malloc(BUFF_SIZE));

    // Get using "regular" getcwd with non-nullptr
    char *r1 = getcwd(buff1, BUFF_SIZE);

    ASSERT_EQ(r1, buff1);

    // Getcwd with NULL and no size, getcwd should allocate memory
    char *r2 = getcwd(nullptr, 0);
    ASSERT_NE(r2, nullptr);
    ASSERT_EQ(0, strncmp(r1, r2, BUFF_SIZE));

    // Getcwd with NULL but set size, getcwd should allocate memory
    char *r3 = getcwd(nullptr, BUFF_SIZE);
    ASSERT_NE(r3, nullptr);
    EXPECT_EQ(0, strncmp(r1, r3, BUFF_SIZE));

    free(buff1);
    free(r2);
    free(r3);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
