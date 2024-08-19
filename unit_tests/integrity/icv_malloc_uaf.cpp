// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: cc-integrity c3-integrity
// simics_args: enable_integrity=TRUE
// should_fail: yes
// xfail: *

// NOTE: Kernel support not yet extended to Integrity

#include <csignal>
#include <gtest/gtest.h>
#include "malloc/cc_globals.h"
#include "unit_tests/common.h"

static const char *str = "Hello World!";
static const size_t size = 128;

// This is a negative test, and should fail.

TEST(Integrity, test_isa_inv_icv) {
    // Enable integrity
    if (!is_model("native")) {
        cc_set_icv_enabled(true);
    }

    // Get CA since CC_ENABLED=1
    auto ca = reinterpret_cast<char *>(malloc(size));

    // Copy in string and expect it to be readable back
    strncpy(ca, str, size);
    EXPECT_STREQ(ca, str);

    // Clear ICV
    free(ca);

    // Should fail here, UAF
    EXPECT_EXIT(memset(ca, 0x42, size), testing::KilledBySignal(SIGSEGV),
                "Segmentation fault");

    // Disable integrity
    if (!is_model("native")) {
        cc_set_icv_enabled(false);
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
