// model: cc-integrity c3-integrity
// simics_args: disable_cc_env=1
// should_fail: yes

#define DEBUG

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
        assert(!is_encoded_cc_ptr(la) && "Test no working, should be la");
        assert(is_encoded_cc_ptr(ca) && "Test no working, should be ca");
        dbgprint("Setting ICV 0x%016lx + %lu", (uint64_t)ca, size);
        cc_set_icv(ca, size);
    }

    dbgprint("Copying string to 0x%016lx + %lu", (uint64_t)ca, size);
    // Write to ICV-protected memory using CA
    strncpy(ca, str, size);
    EXPECT_STREQ(ca, str);

    dbgprint("Attempt write to 0x%016lx + %lu", (uint64_t)la, size);
    strncpy(la, str, size);
    // EXPECT_DEATH seems to be unreliable in the test environments
    // EXPECT_DEATH(strncpy(la, str, size), ".*");

    // Clean out the ICV
    if (!is_model("native")) {
        cc_set_icv(ca, size);
    }

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
