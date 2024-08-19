// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: *
// nomodel: lim -integrity
#include <unistd.h>
#include <gtest/gtest.h>
#include "unit_tests/common.h"

using namespace std;

static inline void test_decryption(uint64_t *const c_ptr) {
    // const char c = 'A';
    const uint64_t c = 0xDEADBEEF;
    // MAGIC(0);
    *c_ptr = c;
    // MAGIC(0);

    ASSERT_EQ(*c_ptr, c);

    uint64_t *dec_c_ptr = cc_dec_if_encoded_ptr(c_ptr);

    // fprintf(stderr, "c_ptr:      %016lx\n", (uint64_t) c_ptr);
    // fprintf(stderr, "dec_c_ptr:  %016lx\n", (uint64_t) dec_c_ptr);
    // fprintf(stderr, "*c_ptr:     %016lx\n", (uint64_t) *c_ptr);
    // fprintf(stderr, "*dec_c_ptr: %016lx\n", (uint64_t) *dec_c_ptr);

    // Make sure we can read back the same value
    ASSERT_TRUE(*c_ptr == c);
    // Make sure the *LA and *CA values differ (if c_ptr is CA)
    ASSERT_TRUE(!is_encoded_cc_ptr(c_ptr) ||
                (c_ptr != dec_c_ptr || *c_ptr != *dec_c_ptr));

    // Make sure write to *LA garbles *CA (if c_ptr is CA)
    const uint64_t c_old = *c_ptr;
    *dec_c_ptr = c;
    ASSERT_TRUE(!is_encoded_cc_ptr(c_ptr) || *c_ptr != c);
}

TEST(DecryptPtrTest, heap) {
    uint64_t *c_ptr = (uint64_t *)malloc(sizeof(uint64_t));
    test_decryption(c_ptr);
}

TEST(DecryptPtrTest, stack) {
    uint64_t c;
    uint64_t *c_ptr = &c;

    test_decryption(c_ptr);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
