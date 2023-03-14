// model: *
#include <gtest/gtest.h>
/*
 * Just to sanity-check that gtest builds work in the first place...
 */

#include "unit_tests/common.h"

#define SRAND_SEED 0

TEST(HELLO, HelloWorld) {
    printf("Hello World\n");
    ASSERT_TRUE(true);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
