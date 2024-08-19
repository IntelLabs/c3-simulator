// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: *
// skip: zts
// xfail: -integrity
#include <malloc.h>
#include <xmmintrin.h>
#include <gtest/gtest.h>

TEST(Utils, MallocUsableSize) {
    srand(1234);
    size_t max_size = 10000;
    for (unsigned int i = 0; i < max_size; i++) {
        size_t size = 1 + (((size_t)rand()) % max_size);
        uint8_t *p = (uint8_t *)malloc(size);
        ASSERT_LE(size, malloc_usable_size(p));
        free(p);
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
