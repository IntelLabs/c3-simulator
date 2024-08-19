// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: cc-integrity c3-integrity
// simics_args: enable_integrity=TRUE
// should_fail: yes

#include <gtest/gtest.h>
#include "malloc/cc_globals.h"
#include "unit_tests/common.h"

#define INV_OFFSET 16

static const char *str = "Hello World AAAAAAAAAAAAAAAAAA!";

// This is a negative test, and should fail.

TEST(Integrity, test_icv_rep_movs) {
    // if (!is_model("native")) {
    //     cc_set_icv_enabled(true);
    // }
    char *src = (char *)malloc(128);
    char *dst = (char *)malloc(128);
    // Copy over example string to CA src
    strncpy(src, str, 128);

    printf("The string at src (%p): '%s'\n", src, src);
    printf("Now invalidating src (%p) + %d\n", src, INV_OFFSET);
    *(src + INV_OFFSET - 1) = 0;
    cc_isa_invicv((src + INV_OFFSET));
    printf("The string at src (after invalidation): '%s'\n", src);

    int n = strlen(str) + 1;
    printf("Copying string at src to dst with ICVs preserved\n");
    printf("DS REP MOVS src=%p dst=%p, n=%d\n", src, dst, n);
    cc_icv_memcpy(dst, src, n);
    printf("The string at src (%p): '%s'\n", src, src);
    printf("The string at dst (%p): '%s'\n", dst, dst);
    EXPECT_STREQ(src, dst);
    printf("Now write to dst (%p) + %d (expecting ICV mismatch at %p)\n", dst,
           INV_OFFSET, dst + INV_OFFSET);
    // Avoid EXPECT_DEATH since ICVs seem to interact badly with it
    // EXPECT_DEATH((*(dst + INV_OFFSET) = 0), ".*");
    *(dst + INV_OFFSET) = 0;
    printf("ERROR: no fault on write to ICV mismatch. Should have failed.\n");
    free(src);
    free(dst);

    // if (!is_model("native")) {
    //     cc_set_icv_enabled(false);
    // }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
