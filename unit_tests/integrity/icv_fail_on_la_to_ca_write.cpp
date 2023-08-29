// model: cc-integrity c3-integrity
// simics_args: disable_cc_env=1

// NOTE: Kernel support not yet extended to Integrity

// #define DEBUG

#include <gtest/gtest.h>
#include "malloc/cc_globals.h"
#include "unit_tests/common.h"

static const char *str = "Hello World!";
static const size_t size = 128;

TEST(Integrity, icv_fail_on_la_to_ca_write) {
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

    // Set ICV for allocation
    if (!is_model("native")) {
        dbgprint("Setting ICV 0x%016lx + %lu", (uint64_t)ca, size);
        cc_set_icv(ca, size);
    }

    // Write to ICV-protected memory using CA
    strncpy(ca, str, size);
    EXPECT_STREQ(ca, str);

    // Write to ICV-protected buffer using LA!
    EXPECT_DEATH(strncpy(la, str, size), ".*");

    // Clean out the ICV
    if (!is_model("native")) {
        cc_set_icv(ca, size);
    }

    // Disable integrity
    if (!is_model("native")) {
        cc_set_icv_enabled(false);
    }

    // This should still work now since the EXPECT_DEATH gets suppressed here
    EXPECT_STREQ(ca, str);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
