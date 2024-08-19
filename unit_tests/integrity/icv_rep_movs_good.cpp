// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: cc-integrity c3-integrity
// simics_args: enable_integrity=TRUE

#include <gtest/gtest.h>
#include "malloc/cc_globals.h"
#include "unit_tests/common.h"

#define INV_OFFSET 16

static const char *str = "Hello World AAAAAAAAAAAAAAAAAA!";

// This is a negative test, and should fail.

TEST(Integrity, test_icv_rep_movs) {
    char *orig_src = (char *)malloc(128);
    char *orig_dst = (char *)malloc(128);
    char *src = orig_src;
    char *dst = orig_dst;
    // Copy over example string to CA src
    strncpy(src, str, 128);

    printf("The string at src (%p): '%s'\n", src, src);
    printf("Now invalidating src (%p) + %d\n", src, INV_OFFSET);
    *(src + INV_OFFSET - 1) = 0;
    cc_isa_invicv((src + INV_OFFSET));
    printf("The string at src (after invalidation): '%s'\n", src);

    int n = strlen(str) + 1;
    *(src + n) = 0;
    printf("Copying string at src to dst with ICVs preserved\n");
    printf("DS REP MOVS src=%p dst=%p, n=%d\n", src, dst, n);
    cc_icv_memcpy(dst, src, 128);
    printf("The string at src (%p): '%s'\n", src, src);
    printf("The string at dst (%p): '%s'\n", dst, dst);
    EXPECT_STREQ(src, dst);
    src += INV_OFFSET + 1;
    dst += INV_OFFSET + 1;
    printf("The string at src (%p): '%s'\n", src, src);
    printf("The string at dst (%p): '%s'\n", dst, dst);
    EXPECT_STREQ(src, dst);
    free(orig_src);
    free(orig_dst);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
