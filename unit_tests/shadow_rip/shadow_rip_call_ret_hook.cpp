// model: cc
// no_kernel: yes
// nomodel: -zts
// xfail: -castack

// NOTE: Kernel support not yet extended to GSRIP
// NOTE: Stack hardening not yet compatible with GSRIP

// #define DEBUG

#include <math.h>
#include <gtest/gtest.h>
#include "unit_tests/common.h"
#include "unit_tests/shadow_rip_misc.h"

const bool enable_shadow_rip = is_model("cc");

typedef void *(*get_rip_rel_lea_ptr_type)(void);

void *(*g_get_rip_rel_lea_ptr)(void);

#define asm_lea(offset, res)                                                   \
    do {                                                                       \
        asm volatile("lea " #offset "(%%rip), %[_res]\n\t;"                    \
                     : [_res] "+r"(res)                                        \
                     :                                                         \
                     :);                                                       \
    } while (0)

__attribute__((noinline)) void *get_rip_rel_lea() {
    void *ptr = nullptr;
    asm_lea(0x10, ptr);
    asm("nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        :
        :
        :);
    return ptr;
}

__attribute__((noinline)) std::pair<void *, void *>
nested_rip_rel(void *(*func)(void)) {
    void *ptr1 = nullptr;
    asm_lea(0x10, ptr1);
    asm("nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        :
        :
        :);
    void *ptr2 = func();
    return std::make_pair<>(ptr1, ptr2);  // NOLINT
}

TEST(ShadowRip, call_ret) {
    // Ugly trick to get gsrip that covers the get_rip_rel_lea function
    auto gsrip = reinterpret_cast<uint64_t>(
            gen_gsrip_range(&get_rip_rel_lea, &get_rip_rel_lea, 64));

    auto get_rip_rel_lea_ptr = reinterpret_cast<void *(*)(void)>(gsrip);

    // dbgprint("ptr    0x%016lx", (uint64_t)ptr);
    dbgprint("gsrip  0x%016lx", (uint64_t)gsrip);

    // Load once before configuring custom shadow rip
    void *la = get_rip_rel_lea();

    // Enable shadow rip, but don't set its value!
    dbgprint("Enabling shadow_rip");
    set_and_enable_shadow_rip<uint64_t>(0);

    // Now do indirect call using the CA with the shadow rip!
    dbgprint("Indirect call with CA: %p", get_rip_rel_lea_ptr);
    void *gsripped_la = get_rip_rel_lea_ptr();

    dbgprint("Done, disabling shadow_rip");
    set_cpu_shadow_rip_enabled(false);

    // The LEA results should be different now if call / ret set gsrip
    ASSERT_NE(gsripped_la, la);
}

TEST(ShadowRip, nested_call_ret) {
    // Get reference value
    auto pair1 = nested_rip_rel(&get_rip_rel_lea);

    // Prepare gsrips
    auto gsrip1 = reinterpret_cast<uint64_t>(
            gen_gsrip_range(&nested_rip_rel, &nested_rip_rel, 64));
    auto gsrip2 = reinterpret_cast<uint64_t>(
            gen_gsrip_range(&get_rip_rel_lea, &get_rip_rel_lea, 64));

    // Prepare function pointers
    auto f1 = reinterpret_cast<std::pair<void *, void *> (*)(void *(*)(void))>(
            gsrip1);
    auto f2 = reinterpret_cast<void *(*)(void)>(gsrip2);

    dbgprint("gsrip1: 0x%016lx", gsrip1);
    dbgprint("gsrip1: 0x%016lx", gsrip2);

    set_and_enable_shadow_rip<uint64_t>(0);
    auto pair2 = f1(f2);
    set_cpu_shadow_rip_enabled(false);

    ASSERT_NE(pair2.first, pair1.first);
    ASSERT_NE(pair2.second, pair1.second);

    auto pair3 = nested_rip_rel(&get_rip_rel_lea);
    // Check we got differnt values from the reference values
    ASSERT_NE(pair3.first, pair2.first);
    ASSERT_NE(pair3.second, pair2.second);
    ASSERT_EQ(pair3.first, pair1.first);
    ASSERT_EQ(pair3.second, pair1.second);

    set_and_enable_shadow_rip<uint64_t>(0);
    auto pair4 = nested_rip_rel(f2);
    set_cpu_shadow_rip_enabled(false);

    ASSERT_EQ(pair4.first, pair1.first);
    ASSERT_NE(pair4.second, pair1.second);
    ASSERT_EQ(pair4.second, pair2.second);
}

__attribute__((noinline)) void *
do_call_mem_ptr_ret(void *(**get_rip_rel_lea_ptr)(void)) {
    // Enable shadow rip, but don't set its value!
    set_and_enable_shadow_rip<uint64_t>(0);

    // Now call the CA with the shadow rip!
    void *gsripped_la = (*get_rip_rel_lea_ptr)();

    set_cpu_shadow_rip_enabled(false);

    return gsripped_la;
}

TEST(ShadowRip, call_mem_ptr_ret) {
    // Ugly trick to get gsrip that covers the get_rip_rel_lea function
    auto gsrip = reinterpret_cast<uint64_t>(
            gen_gsrip_range(&get_rip_rel_lea, &get_rip_rel_lea, 64));

    g_get_rip_rel_lea_ptr = reinterpret_cast<void *(*)(void)>(gsrip);

    // dbgprint("ptr    0x%016lx", (uint64_t)ptr);
    dbgprint("gsrip  0x%016lx", (uint64_t)gsrip);

    // Load once before configuring custom shadow rip
    void *la = get_rip_rel_lea();

    // Now call the CA with the shadow rip!
    void *gsripped_la = do_call_mem_ptr_ret(&g_get_rip_rel_lea_ptr);

    // The LEA results should be different now if call / ret set gsrip
    ASSERT_NE(gsripped_la, la);
}

TEST(ShadowRip, call_global_mem_ret) {
    // Ugly trick to get gsrip that covers the get_rip_rel_lea function
    auto gsrip = reinterpret_cast<uint64_t>(
            gen_gsrip_range(&get_rip_rel_lea, &get_rip_rel_lea, 64));

    g_get_rip_rel_lea_ptr = reinterpret_cast<void *(*)(void)>(gsrip);

    // dbgprint("ptr    0x%016lx", (uint64_t)ptr);
    dbgprint("gsrip  0x%016lx", (uint64_t)gsrip);

    // Load once before configuring custom shadow rip
    void *la = get_rip_rel_lea();

    // Enable shadow rip, but don't set its value!
    set_and_enable_shadow_rip<uint64_t>(0);

    // Now call the CA with the shadow rip!
    void *gsripped_la = g_get_rip_rel_lea_ptr();

    set_cpu_shadow_rip_enabled(false);

    // The LEA results should be different now if call / ret set gsrip
    ASSERT_NE(gsripped_la, la);
}

TEST(ShadowRip, call_reg) {
    // Ugly trick to get gsrip that covers the get_rip_rel_lea function
    auto gsrip = reinterpret_cast<uint64_t>(
            gen_gsrip_range(&get_rip_rel_lea, &get_rip_rel_lea, 64));

    g_get_rip_rel_lea_ptr = reinterpret_cast<void *(*)(void)>(gsrip);
    const uint64_t check_val = gsrip;

    // dbgprint("ptr    0x%016lx", (uint64_t)ptr);
    dbgprint("gsrip  0x%016lx", (uint64_t)gsrip);

    // Load once before configuring custom shadow rip
    void *la = get_rip_rel_lea();

    // Enable shadow rip, but don't set its value!
    set_and_enable_shadow_rip<uint64_t>(0);

    // Now call the CA with the shadow rip!
    void *gsripped_la = g_get_rip_rel_lea_ptr();
    asm volatile("mov %[ptr], %%r12 \n"
                 "call %%r12        \n"
                 "mov %%r12, %[ptr] \n"
                 : [ptr] "+r"(g_get_rip_rel_lea_ptr)
                 :
                 : "rax", "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11",
                   "cc", "memory");

    set_cpu_shadow_rip_enabled(false);

    // The g_get_rip_Rel_lea_ptr function pointer should be the same as its
    // value was stored in the callee-saved r12 register.
    ASSERT_EQ(check_val, (uint64_t)g_get_rip_rel_lea_ptr);

    // The LEA results should be different now if call / ret set gsrip
    ASSERT_NE(gsripped_la, la);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    if (!enable_shadow_rip) {
        dbgprint("WARNING: Leaving shadow rip disabled for model %s", c3_model);
    }
    return RUN_ALL_TESTS();
}
