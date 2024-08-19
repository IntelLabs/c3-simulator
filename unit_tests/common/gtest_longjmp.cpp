// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: *
#include <setjmp.h>
#include <gtest/gtest.h>
using namespace std;  // NOLINT

#define __the_longjmp(...) longjmp(__VA_ARGS__)
#define __the_setjmp(...) setjmp(__VA_ARGS__)

void do_the_jump(jmp_buf *j, int *p, int v) {
    *p += 1;
    __the_longjmp(*j, v);
}

void deep_do_the_jump(jmp_buf *j, int *p, int v) {
    *p += 1;
    --v;
    if (v > 0)
        deep_do_the_jump(j, p, v);
    else
        do_the_jump(j, p, 100);
}

__attribute__((noinline)) void adder(int *p, int v) { *p += v; }

TEST(LONGJMP, local_buf_on_stack) {
    jmp_buf j;
    int a = 10;

    int r = __the_setjmp(j);

    if (r == 0) {
        ASSERT_EQ(a, 10);
        a += 1;
        ASSERT_EQ(a, 11);
        adder(&a, 100);
        ASSERT_EQ(a, 111);
        __the_longjmp(j, 1000), a += 99999;
    } else {
        a += r;
    }

    ASSERT_EQ(a, 1111);
}

TEST(LONGJMP, local_buf_on_heap) {
    jmp_buf *j = (jmp_buf *)malloc(sizeof(jmp_buf));
    int a = 10;

    int r = __the_setjmp(*j);

    if (r == 0) {
        ASSERT_EQ(a, 10);
        a += 1;
        ASSERT_EQ(a, 11);
        adder(&a, 100);
        ASSERT_EQ(a, 111);
        __the_longjmp(*j, 1000), a += 99999;
    } else {
        a += r;
    }

    ASSERT_EQ(a, 1111);
}

// Jump one stack frame back
TEST(LONGJMP, longjmp_oneframe) {
    jmp_buf j;
    int a = 10;

    int r = __the_setjmp(j);

    if (r == 0) {
        do_the_jump(&j, &a, 100);
        ASSERT_EQ(false, true);
    } else {
        a += r;
    }

    ASSERT_EQ(a, 111);
}

// Jump multiple stack frames back
TEST(LONGJMP, longjmp_manyframes) {
    jmp_buf j;
    int a = 1;

    int r = __the_setjmp(j);

    if (r == 0) {
        deep_do_the_jump(&j, &a, 9);
        ASSERT_EQ(false, true);
    } else {
        a += r;
    }

    ASSERT_EQ(a, 111);
}

// Jump one stack frame back
TEST(LONGJMP, longjmp_oneframe_heap) {
    jmp_buf *j = (jmp_buf *)malloc(sizeof(jmp_buf));
    int a = 10;

    int r = __the_setjmp(*j);

    if (r == 0) {
        do_the_jump(j, &a, 100);
        ASSERT_EQ(false, true);
    } else {
        a += r;
    }

    ASSERT_EQ(a, 111);
}

// Jump multiple stack frames back
TEST(LONGJMP, longjmp_manyframes_heap) {
    jmp_buf *j = (jmp_buf *)malloc(sizeof(jmp_buf));
    int a = 1;

    int r = __the_setjmp(*j);

    if (r == 0) {
        deep_do_the_jump(j, &a, 9);
        ASSERT_EQ(false, true);
    } else {
        a += r;
    }

    ASSERT_EQ(a, 111);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
