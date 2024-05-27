// model: cc-integrity c3-integrity
// simics_args: enable_integrity=1
// should_fail: yes

// NOTE: Kernel support not yet extended to Integrity

#include <gtest/gtest.h>
#include "malloc/cc_globals.h"
#include "unit_tests/common.h"

static const size_t size = sizeof(int);

// This is the positive test, and should succeed.

TEST(Integrity, test_isa_init_icv_fail_uninit_use) {
    // Get regular LA since we've removed CC_ENABLED=1
    auto la = reinterpret_cast<int *>(malloc(size));
    auto ca = la;

    // Now generate CA
    if (!is_model("native")) {
        ca = cc_isa_encptr(la, size);
    }

    // Enable integrity
    if (!is_model("native")) {
        cc_set_icv_enabled(true);
    }

    EXPECT_DEATH(*ca = 12, ".*");

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
