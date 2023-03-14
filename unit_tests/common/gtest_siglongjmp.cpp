// model: *

#include <setjmp.h>
#include <gtest/gtest.h>
using namespace std;

#define __the_longjmp(...) siglongjmp(__VA_ARGS__)
#define __the_setjmp(x) sigsetjmp(x, 1)

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

// Jump one stack frame back
TEST(SIGLONGJMP, longjmp_oneframe) {
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
TEST(SIGLONGJMP, longjmp_manyframes) {
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
TEST(SIGLONGJMP, longjmp_oneframe_heap) {
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
TEST(SIGLONGJMP, longjmp_manyframes_heap) {
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
