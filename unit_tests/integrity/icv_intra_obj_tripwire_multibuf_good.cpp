// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: cc-integrity-intra
// need_kernel: yes
// simics_args: enable_integrity=TRUE
// cxx_flags: -fuse-ld=lld -finsert-intraobject-tripwires=all

#include <gtest/gtest.h>
#include "malloc/cc_globals.h"
#include "unit_tests/common.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wwritable-strings"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SRC_STR "0123456789abcdef0123456789abcde"

typedef struct _charVoid {
    char charFirst[16];
    void *voidSecond;
    char charSecond[16];
    void *voidThird;
} charVoid;

// This is a negative test, and should fail.
TEST(Integrity, test_icv_intra_obj_tripwire_multibuf_good) {
    if (!is_model("native")) {
        cc_set_icv_enabled(true);
    }

    charVoid *structCharVoid = (charVoid *)malloc(sizeof(charVoid));

    if (structCharVoid == NULL) {
        exit(-1);
    }
    structCharVoid->voidSecond = (void *)SRC_STR;
    /* Print the initial block pointed to by structCharVoid->voidSecond */
    printf("struct second: %x write size %d\n",
           (char *)structCharVoid->voidSecond, sizeof(*structCharVoid));
    /* FLAW: Use the sizeof(*structCharVoid) which will overwrite the pointer y
     */
    memcpy(structCharVoid->charFirst, SRC_STR,
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

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
