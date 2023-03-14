// model: *

// #define DEBUG
#include <cmath>
#include <cstring>
#include <gtest/gtest.h>
#include "unit_tests/common.h"

TEST(STRING, heap_strnlen) {
    for (size_t i = 4; i <= 8; i++) {
        size_t len = pow(2, i);
        char *str = reinterpret_cast<char *>(malloc(len));

        // Set string to all-1 (no terminating \0)
        memset(str, 123, len);

        str[len - 1] = 0;
        dbgprint("Tryint strlen %lu - 1", len);
        ASSERT_EQ(len - 1, strnlen(str, len));

        str[len - 1] = 123;
        dbgprint("Tryint strlen %lu", len);
        // MAGIC(0);
        ASSERT_EQ(len, strnlen(str, len));

        free(str);
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
