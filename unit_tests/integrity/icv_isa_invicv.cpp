// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: cc
// nomodel: zts -zts -castack
// simics_args: disable_cc_env=TRUE enable_integrity=TRUE

// NOTE: Kernel support not yet extended to Integrity

// #define DEBUG

#include <gtest/gtest.h>
#include "malloc/cc_globals.h"
#include "unit_tests/common.h"

static const char *str = "Hello World!";
static const size_t size = 128;

// This is the positive test, and should succeed.

TEST(Integrity, test_isa_inv_icv) {
    // Get regular LA since we've removed CC_ENABLED=1
    auto la = reinterpret_cast<char *>(malloc(size));
    auto ca = la;

    // Now generate CA
    if (!is_model("native")) {
        ca = cc_isa_encptr(la, size);
    }

    // Enable integrity
    if (!is_model("native")) {
        cc_set_icv_enabled(true);
    }

    // Set ICV for allocation
    if (!is_model("native")) {
        dbgprint("Setting ICV for 0x%016lx (0x%016lx)", ca, la);
        cc_set_icv(ca, size);
    }

    // Copy in string and expect it to be readable back
    strncpy(ca, str, size);
    EXPECT_STREQ(ca, str);

    cc_isa_invicv(ca);

    // Clear ICV
    if (!is_model("native")) {
        dbgprint("Clearing ICV for 0x%016lx (0x%016lx)", ca, cc_isa_decptr(ca));
        cc_set_icv(cc_isa_decptr(ca), size);
    }

    // Disable integrity
    if (!is_model("native")) {
        cc_set_icv_enabled(false);
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
