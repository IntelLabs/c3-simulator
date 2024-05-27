// model: cc
// nomodel: zts -zts -castack
// simics_args: disable_cc_env=1 enable_integrity=1

// NOTE: Kernel support not yet extended to Integrity

// This is the negative test, and should fail.
// it represents a UAF

#include <gtest/gtest.h>
#include "malloc/cc_globals.h"
#include "unit_tests/common.h"

static const char *str = "Hello World!";
static const size_t size = 128;

TEST(Integrity, test_isa_inv_icv_fail_uaf) {
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
        cc_set_icv(ca, size);
    }

    // Copy in string and expect it to be readable back
    strncpy(ca, str, size);
    EXPECT_STREQ(ca, str);

    // invalidate the integrity of the first granule of the string
    cc_isa_invicv(ca);

    // performing the same string copy should now fail since the integrity is
    // invalidated.
    EXPECT_DEATH(strncpy(ca, str, size), ".*");

    // FIXME, tried to get EXPECT DEATh to work with uint64 malloc'd but kept
    // segfaulting

    // Clear ICV
    if (!is_model("native")) {
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
