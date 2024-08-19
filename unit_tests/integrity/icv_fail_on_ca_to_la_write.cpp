// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: cc-integrity c3-integrity
// simics_args: disable_cc_env=TRUE
// should_fail: yes

#define DEBUG

#include <gtest/gtest.h>
#include "malloc/cc_globals.h"
#include "unit_tests/common.h"

static const char *str = "Hello World!";
static const size_t size = 128;

TEST(Integrity, icv_fail_on_ca_to_la_write) {
    // Get regular LA since we've removed CC_ENABLED=1
    auto la = reinterpret_cast<char *>(malloc(size));
    auto ca = la;

    // Get a CA
    if (!is_model("native")) {
        ca = cc_isa_encptr(la, size);
    }

    // Enable integrity
    if (!is_model("native")) {
        cc_set_icv_enabled(true);
    }

    // Write to plaintext buffer using LA
    strncpy(la, str, size);
    EXPECT_STREQ(la, str);

    // Write to plaintext buffer using CA!
    strncpy(ca, str, size);
    // EXPECT_DEATH seems to be unreliable in the test environments
    // EXPECT_DEATH(strncpy(ca, str, size), ".*");

    // Disable integrity
    if (!is_model("native")) {
        cc_set_icv_enabled(false);
    }

    printf("FAILURE: Should never reach here!!!\n");
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
