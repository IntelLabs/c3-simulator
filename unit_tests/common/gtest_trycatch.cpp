// model: *
// need_libunwind: yes
#include <gtest/gtest.h>
/*
 * Just to sanity-check that gtest builds work in the first place...
 */

#define MAGIC(n)                                                               \
    do {                                                                       \
        int simics_magic_instr_dummy;                                          \
        __asm__ __volatile__("cpuid"                                           \
                             : "=a"(simics_magic_instr_dummy)                  \
                             : "a"(0x4711 | ((unsigned)(n) << 16))             \
                             : "ecx", "edx", "ebx");                           \
    } while (0)

#define SRAND_SEED 0

class DestructMe {
 private:
    int *val;

 public:
    explicit DestructMe(int *val) : val(val) {}
    ~DestructMe() { *val += 10000; }
    virtual void doInc(int n) { *val += n; }
};

__attribute__((noinline)) void thrower(int *val, int i, int j) {
    if (i > 0)
        throw 1000;
    *val = j;
}

__attribute__((noinline)) void caller(void (*func)(int *, int, int), int *val,
                                      int i, int j) {
    *val += i;
    int old_val = *val;
    // We expec this to throw and not change val
    ASSERT_ANY_THROW(func(val, i, j));
    ASSERT_EQ(*val, old_val);
    func(val, i, j);
}

__attribute__((noinline)) void caller_with_obj(void (*func)(int *, int, int),
                                               int *val, int i, int j) {
    DestructMe obj{val};
    caller(func, val, i, j);
    obj.doInc(879321465);
}

TEST(TRYCATCH, throwint_onefunc) {
    int val = 1;
    try {
        val += 10;
        throw 100;
        val = 123;
    } catch (int e) {
        val += e;
    }
    ASSERT_EQ(val, 111);
}

TEST(TRYCATCH, throwint_indirect) {
    int val = 0;
    ASSERT_ANY_THROW(thrower(&val, 1, 1));
    ASSERT_EQ(val, 0);
    ASSERT_NO_THROW(thrower(&val, -1, 2));
    ASSERT_EQ(val, 2);
    val = 1;

    try {
        val += 10;
        caller(&thrower, &val, 100, 123);
        val = 123;
    } catch (int e) {
        val += e;
    }
    ASSERT_EQ(val, 1111);
}

TEST(TRYCATCH, throwint_indirect_destructors) {
    int val = 0;
    ASSERT_ANY_THROW(thrower(&val, 1, 1));
    ASSERT_EQ(val, 0);
    ASSERT_NO_THROW(thrower(&val, -1, 2));
    ASSERT_EQ(val, 2);
    val = 1;

    try {
        val += 10;
        caller_with_obj(&thrower, &val, 100, 123);
        val = 123;
    } catch (int e) {
        val += e;
    }
    ASSERT_EQ(val, 11111);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
