// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: *
// nomodel: lim -integrity
// xfail: zts -castack -zts
// TODO(hliljest): test currently relies on heap, should extend to stack-only

#include <unistd.h>
#include <cstdlib>
#include <gtest/gtest.h>
#include "malloc/cc_globals.h"
#include "unit_tests/common.h"

#define STR "Hello World!\n"
#define STR_LEN sizeof("Hello World!\n")

TEST(TCLASS, test_key_configuration) {
    // Get a plaintext buffer (i.e., using regular LA)
    char *plaintext_buff = (char *)cc_dec_if_encoded_ptr(malloc(STR_LEN));

    char *buff = (char *)malloc(STR_LEN);
    strncpy(buff, STR, STR_LEN);

    char *plaintext_ptr = (char *)cc_dec_if_encoded_ptr((void *)buff);

    if (plaintext_ptr == buff) {
        fprintf(stderr, "No pointer encryption? Skipping....\n");
        return;
    }

    // Expect we get different data via plaintext LA
    ASSERT_NE(0, std::memcmp(plaintext_ptr, buff, STR_LEN));

    // Save the old C3 context
    struct cc_context old_ctx;
    cc_save_context(&old_ctx);

    // Get a separate copy and use it to change private key
    struct cc_context new_ctx;
    cc_save_context(&new_ctx);
    new_ctx.dp_key_bytes_[0]++;
    new_ctx.ds_key_bytes_[0]++;
    cc_load_context(&new_ctx);

    // Then copy to our plaintext buffer
    for (int i = 0; i < STR_LEN; ++i) {
        plaintext_buff[i] = buff[i];
    }

    // Restore original C3 context
    cc_load_context(&old_ctx);

    MAGIC(0);

    // Check we read different things after data keys were changed
    ASSERT_NE(0, std::memcmp(plaintext_buff, buff, STR_LEN));
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    srand(123);
    return RUN_ALL_TESTS();
}
