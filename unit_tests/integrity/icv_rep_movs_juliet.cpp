// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: cc-integrity c3-integrity
// simics_args: enable_integrity=TRUE
// should_fail: yes

#include <gtest/gtest.h>
#include "malloc/cc_globals.h"
#include "unit_tests/common.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wwritable-strings"

#define INV_OFFSET 16
#define MAGIC_VAL_INTRA 0xdeadbeefd0d0caca

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SRC_STR "0123456789abcdef0123456789abcde"

typedef struct _charVoid {
    char tripwire1[8];
    char charFirst[16];
    char tripwire2[8];
    void *voidSecond;
    void *voidThird;
} charVoid;

TEST(Integrity, test_icv_rep_movs_juliet_good) {
    if (!is_model("native")) {
        cc_set_icv_enabled(true);
    }

    charVoid *structCharVoid = (charVoid *)malloc(sizeof(charVoid));

    // Mock tripwires
    uint64_t *tripwire = (uint64_t *)structCharVoid->tripwire1;
    *tripwire = MAGIC_VAL_INTRA;
    cc_isa_invicv(tripwire);
    tripwire = (uint64_t *)structCharVoid->tripwire2;
    *tripwire = MAGIC_VAL_INTRA;
    cc_isa_invicv(tripwire);
    // END mock tripwires

    if (structCharVoid == NULL) {
        exit(-1);
    }
    structCharVoid->voidSecond = (void *)SRC_STR;
    /* Print the initial block pointed to by structCharVoid->voidSecond */
    printf("struct second: %x write size %d\n",
           (char *)structCharVoid->voidSecond, sizeof(*structCharVoid));
    /* FLAW: Use the sizeof(*structCharVoid) which will overwrite the pointer y
     */
    cc_icv_memcpy(structCharVoid->charFirst, SRC_STR,
                  sizeof(structCharVoid->charFirst));

    structCharVoid
            ->charFirst[(sizeof(structCharVoid->charFirst) / sizeof(char)) -
                        1] = '\0'; /* null terminate the string */
    printf("struct first: %x\n", (char *)structCharVoid->charFirst);
    printf("struct second: %x\n", (char *)structCharVoid->voidSecond);
    free(structCharVoid);

    printf("- Done with juliet_good - \n");

    if (!is_model("native")) {
        cc_set_icv_enabled(false);
    }
}

// This is a negative test, and should fail.
TEST(Integrity, test_icv_rep_movs_juliet_z_bad) {
    if (!is_model("native")) {
        cc_set_icv_enabled(true);
    }

    charVoid *structCharVoid = (charVoid *)malloc(sizeof(charVoid));

    // Mock tripwires
    uint64_t *tripwire = (uint64_t *)structCharVoid->tripwire1;
    *tripwire = MAGIC_VAL_INTRA;
    cc_isa_invicv(tripwire);
    tripwire = (uint64_t *)structCharVoid->tripwire2;
    *tripwire = MAGIC_VAL_INTRA;
    cc_isa_invicv(tripwire);
    // END mock tripwires

    if (structCharVoid == NULL) {
        exit(-1);
    }
    structCharVoid->voidSecond = (void *)SRC_STR;
    /* Print the initial block pointed to by structCharVoid->voidSecond */
    printf("struct second: %x write size %d\n",
           (char *)structCharVoid->voidSecond, sizeof(*structCharVoid));
    /* FLAW: Use the sizeof(*structCharVoid) which will overwrite the pointer y
     */
    // Avoid EXPECT_DEATH since ICVs seem to interact badly with it
    // EXPECT_DEATH(cc_icv_memcpy(structCharVoid->charFirst, SRC_STR,
    //                            sizeof(*structCharVoid)),
    cc_icv_memcpy(structCharVoid->charFirst, SRC_STR, sizeof(*structCharVoid));
    structCharVoid
            ->charFirst[(sizeof(structCharVoid->charFirst) / sizeof(char)) -
                        1] = '\0'; /* null terminate the string */
    printf("struct first: %x\n", (char *)structCharVoid->charFirst);
    printf("struct second: %x\n", (char *)structCharVoid->voidSecond);
    free(structCharVoid);

    printf("- ERROR: Done with juliet_bad. Should not reach this - \n");

    if (!is_model("native")) {
        cc_set_icv_enabled(false);
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
