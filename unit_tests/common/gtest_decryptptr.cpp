// model: *
// nomodel: lim
#include <unistd.h>
#include <gtest/gtest.h>
#include "unit_tests/common.h"

using namespace std;

static inline void test_decryption(char *const c_ptr) {
    const char c = 'A';
    // MAGIC(0);
    *c_ptr = c;
    // MAGIC(0);

    ASSERT_EQ(*c_ptr, c);

    char *dec_c_ptr = cc_dec_if_encoded_ptr(c_ptr);

    // fprintf(stderr, "c_ptr:      %016lx\n", (uint64_t) c_ptr);
    // fprintf(stderr, "dec_c_ptr:  %016lx\n", (uint64_t) dec_c_ptr);
    // fprintf(stderr, "*c_ptr:     %016lx\n", *c_ptr);
    // fprintf(stderr, "*dec_c_ptr: %016lx\n", *dec_c_ptr);

    // Make sure we can read back the same value
    ASSERT_TRUE(*c_ptr == c);
    // Make sure the *LA and *CA values differ (if c_ptr is CA)
    ASSERT_TRUE(!is_encoded_cc_ptr(c_ptr) ||
                (c_ptr != dec_c_ptr || *c_ptr != *dec_c_ptr));

    // Make sure write to *LA garbles *CA (if c_ptr is CA)
    const char c_old = *c_ptr;
    *dec_c_ptr = c;
    ASSERT_TRUE(!is_encoded_cc_ptr(c_ptr) || *c_ptr != c);
}

TEST(DecrypPtrTest, heap) {
    char *c_ptr = (char *)malloc(sizeof(char));
    test_decryption(c_ptr);
}

TEST(DecrypPtrTest, stack) {
    char c;
    char *c_ptr = &c;

    test_decryption(c_ptr);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
