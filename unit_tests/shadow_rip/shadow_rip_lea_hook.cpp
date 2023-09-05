// model: cc
// nomodel: cc-tripwire
// no_kernel: yes
// xfail: *

// NOTE: Kernel support not yet extended to GSRIP, and this is unstable as it
// relies on very specific timing without any interrupts

#define DEBUG

#include <gtest/gtest.h>
#include "unit_tests/common.h"
#include "unit_tests/shadow_rip_misc.h"

#include <math.h>

#define asm_lea_0x10(offset, res)                                              \
    do {                                                                       \
        asm volatile("lea " #offset "(%%rip), %[_res]\n\t;"                    \
                     : [ _res ] "+r"(res)                                      \
                     :                                                         \
                     :);                                                       \
    } while (0)

__attribute__((noinline)) void *get_rip_rel_lea() {
    void *ptr = nullptr;
    asm_lea_0x10(0x10, ptr);
    return ptr;
}

TEST(ShadowRip, detect_lea) {
    void *ptr = nullptr;
    // void *gsrip = malloc(123);

    ptr = get_rip_rel_lea();
    void *gsrip = cc_isa_encptr(cc_dec_if_encoded_ptr(ptr), 1024);

    // dbgprint("ptr    0x%016lx", (uint64_t)ptr);
    // dbgprint("gsrip  0x%016lx", (uint64_t)gsrip);

    ASSERT_NE(ptr, nullptr);
    ASSERT_NE(gsrip, nullptr);

    // Expect the ciphertext bits to differ as ptr is regular RIP-relative
    ASSERT_NE(get_ciphertext(gsrip), get_ciphertext(ptr));

    bool enable_shadow_rip = is_model("cc");

    if (enable_shadow_rip) {
        // dbgprint("Enabling shadow rip for process");
        set_and_enable_shadow_rip(gsrip);
    } else {
        // dbgprint("Leaving shadow rip disabled for model %s", c3_model);
    }

    ptr = get_rip_rel_lea();

    if (enable_shadow_rip) {
        set_cpu_shadow_rip_enabled(false);
    }

    // Expect equal ciphertext bits after GSRIP subsitution by RIP-relative LEA
    ASSERT_EQ(get_ciphertext(gsrip), get_ciphertext(ptr));

    // dbgprint("After  0x%016lx", (uint64_t)ptr);
    ASSERT_NE(ptr, nullptr);

    free(gsrip);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
