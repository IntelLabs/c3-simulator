// model: *
// simics_args: enable_integrity=1

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

typedef struct A_s {
    char arr1[16];
    char arr2[16];
    void *ptr;
    char arr3[16];
    char arr4[16];
} A;

typedef struct B_s {
    char arr[64];
    void *ptr;
} B;

const char *string = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

TEST(Integrity, test_intra_trick_def_use_1) {
    void *b_void = malloc(sizeof(B));
    struct A_s *a = (A *)malloc(sizeof(A));
    a->ptr = b_void;

    printf("a: %p\n", (void *)a);
    printf("b: %p\n", (void *)b_void);

    for (int i = 0; i < 64; ++i) {
        ((B *)b_void)->arr[i] = 'a';
    }
    ((B *)b_void)->arr[63] = 0;

    printf("%s\n", ((B *)b_void)->arr);
}

__attribute__((noinline)) void test_intra_trick_def_use_2_func(void *b_void) {
    for (int i = 0; i < 64; ++i) {
        ((B *)b_void)->arr[i] = 'a';
    }
    ((B *)b_void)->arr[63] = 0;

    printf("%s\n", ((B *)b_void)->arr);
}

TEST(Integrity, test_intra_trick_def_use_2) {
    void *b_void = malloc(sizeof(B));
    struct A_s *a = (A *)malloc(sizeof(A));
    a->ptr = b_void;

    printf("a: %p\n", (void *)a);
    printf("b: %p\n", (void *)b_void);

    test_intra_trick_def_use_2_func(b_void);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
