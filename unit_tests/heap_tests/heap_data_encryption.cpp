// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: cc
// nomodel: -integrity

// #define DEBUG
#include <unistd.h>
#include <cstdlib>
#include <gtest/gtest.h>
#include "unit_tests/common.h"

#define TCLASS DATA_ENCRYPTION_HEAP

using namespace std;  // NOLINT

constexpr const uint64_t sizes[] = {8,       8 * 2,   16 * 16,
                                    32 * 32, 64 * 64, 64 * 1024};
constexpr const size_t num_sizes = sizeof(sizes) / sizeof(uint64_t);

char *memcpy_la(char *to, char *from, size_t len) {
    if (is_encoded_cc_ptr((void *)to))
        to = (char *)cc_isa_decptr(to);

    memcpy(to, from, len);
    to[len - 1] = '\0';
    return to;
}

char *memcpy_ca(char *to, char *from, size_t len) {
    // NOTE: Not always same as CA from malloc due to how len is assigned
    // there!!
    if (!is_encoded_cc_ptr((void *)to))
        to = (char *)cc_isa_encptr(to, len);

    memcpy(to, from, len);
    to[len - 1] = '\0';
    return to;
}

TEST(TCLASS, encrypt_data_1) {
    for (auto size : sizes) {
        char *to = (char *)malloc(size);
        char *from = (char *)malloc(size);
        fill_str(from, size);

        char *la_to = memcpy_la(to, from, size);
        EXPECT_STREQ(la_to, from);

        // Note ca_to might have different power/size than to
        char *ca_to = memcpy_ca(la_to, from, size);
        EXPECT_STREQ(ca_to, from);
    }
}

TEST(TCLASS, encrypt_data_2) {
    for (auto size : sizes) {
        char *to = (char *)malloc(size);
        char *from = (char *)malloc(size);
        fill_str(from, size);

        dbgprint("Looking at 0x%016lx", (uint64_t)to);
        EXPECT_TRUE(is_encoded_cc_ptr(to));

        char *la_to = memcpy_la(to, from, size);

        EXPECT_NE(la_to, to);
        EXPECT_STREQ(la_to, from);

        char *ca_to = memcpy_ca(to, from, size);
        EXPECT_STREQ(ca_to, from);

        EXPECT_NE(ca_to, la_to);
        EXPECT_STRNE(ca_to, la_to);

        auto *decoded_ca_to = (char *)decrypt_ca(ca_to);
        ASSERT_EQ(0, strncmp(decoded_ca_to, la_to, size));
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    srand(123);
    return RUN_ALL_TESTS();
}
