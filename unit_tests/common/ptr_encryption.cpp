// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: cc

// #define DEBUG
#include <unistd.h>
#include <gtest/gtest.h>
#include "unit_tests/common.h"

using namespace std;

constexpr const uint64_t sizes[] = {1,       2,       4,       4 * 4,
                                    16 * 16, 32 * 32, 64 * 64, 64 * 1024};
constexpr const size_t num_sizes = sizeof(sizes) / sizeof(uint64_t);

TEST(ISA_ENCDEC, encptr) {
    void *ptr = (void *)0x5555555LU;
    void *enc_ptr;
    dbg_dump_uint64(ptr);

    for (auto size : sizes) {
        enc_ptr = encrypt_ptr(ptr, size);
        dbg_dump_uint64(enc_ptr);
        ASSERT_NE(ptr, enc_ptr);
    }
}

TEST(ISA_ENCDEC, decptr) {
    void *ptr = (void *)0x5555555LU;
    void *enc_ptr;
    void *dec_ptr;

    dbg_dump_uint64(ptr);

    for (auto size : sizes) {
        enc_ptr = cc_isa_encptr(ptr, size);
        dbg_dump_uint64(enc_ptr);

        dec_ptr = decrypt_ptr(enc_ptr);
        dbg_dump_uint64(dec_ptr);

        ASSERT_NE(ptr, enc_ptr);
        ASSERT_EQ(ptr, dec_ptr);
    }
}

int main(int argc, char **argv) {
    dbgprint("Calling InitGoogleTest");
    testing::InitGoogleTest(&argc, argv);
    dbgprint("Calling RUN_ALL_TESTS");
    return RUN_ALL_TESTS();
}
