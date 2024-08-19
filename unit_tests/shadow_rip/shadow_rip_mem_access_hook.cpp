// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

// model: cc
// nomodel: -integrity -castack -zts
// no_kernel: yes
/*
 Copyright 2023 Intel Corporation
 SPDX-License-Identifier: MIT
*/

// NOTE: Kernel support not yet extended to GSRIP

#define DEBUG

#include <math.h>
#include <algorithm>
#include <list>
#include <gtest/gtest.h>
#include "unit_tests/common.h"
#include "unit_tests/shadow_rip_misc.h"

constexpr uint64_t kInitialGValue = 123456789;

uint64_t g_value = kInitialGValue;
uint64_t g_src = kInitialGValue;
bool g_cond = false;

const bool enable_shadow_rip = is_model("cc");

__attribute__((noinline)) uint64_t load_g_value() { return g_value; }
__attribute__((noinline)) void store_g_value(uint64_t v) { g_value = v; }
__attribute__((noinline)) void copy_src_to_g_value() { g_value = g_src; }

__attribute__((noinline)) uint64_t do_do_cmov(bool cond, uint64_t def_val) {
    // Try to trick the compiler to lower this to a cmov
    return cond ? g_value : def_val;
}
__attribute__((noinline)) uint64_t do_cmov(uint64_t def_val) {
    return do_do_cmov(g_cond, def_val);
}

__attribute__((noinline)) uint64_t do_asm_cmov(bool cond, uint64_t def_val) {
    uint64_t output;
    asm volatile("test   %[cond],%[cond]              \n\t"
                 "mov    %[def_val],%[output]          \n\t"
                 "cmovne 0x10(%%rip),%[output]    \n\t"
                 : [output] "=r"(output)
                 : [def_val] "r"(def_val), [cond] "r"(cond)
                 : "rdi", "rsi", "memory");
    return output;
}

template <typename T> T gen_cag_for(T ptr) {
    std::list<uint64_t> addresses;
    addresses.push_front((uint64_t)cc_dec_if_encoded_ptr(&g_value));
    addresses.push_front((uint64_t)cc_dec_if_encoded_ptr(&g_src));
    addresses.push_front((uint64_t)cc_dec_if_encoded_ptr(&store_g_value));
    addresses.push_front((uint64_t)cc_dec_if_encoded_ptr(&load_g_value));
    addresses.push_front((uint64_t)cc_dec_if_encoded_ptr(&copy_src_to_g_value));
    addresses.push_front((uint64_t)cc_dec_if_encoded_ptr(&do_asm_cmov));
    addresses.push_front((uint64_t)cc_dec_if_encoded_ptr(&do_cmov));

    uint64_t base = *std::min_element(addresses.begin(), addresses.end());
    uint64_t end = *std::max_element(addresses.begin(), addresses.end());
    end += 1024;  // Just to be on the safe side

    return gen_cag(ptr, base, end);
}

TEST(ShadowRip, mem_load) {
    auto la_to_global = cc_dec_if_encoded_ptr(&g_value);
    auto ca_to_global = gen_cag_for(&g_value);
    const auto gsrip = ca_to_global;

    g_value = kInitialGValue;
    ASSERT_EQ(g_value, kInitialGValue);

    auto la_value = *la_to_global;
    auto ca_value = *ca_to_global;

    dbgprint("load_g_value() ->     0x%016lu", load_g_value());
    dbgprint("0x%016lx -> la_value  0x%016lu", la_to_global, la_value);
    dbgprint("0x%016lx -> ca_value  0x%016lu", ca_to_global, ca_value);

    ASSERT_NE(la_value, ca_value);
    ASSERT_EQ(la_value, load_g_value());

    dbgprint("Enabling shadow-rip to read value via global");

    // Enable shadow-rip and set GSRIP to ca_to_global, then try to read
    set_and_enable_shadow_rip(gsrip);

    auto gsrip_loaded_value = load_g_value();

    set_cpu_shadow_rip_enabled(false);

    dbgprint("Disabling shadow-rip");

    dbgprint("gsrip_loaded_value    0x%016lx", gsrip_loaded_value);
    dbgprint("0x%016lx -> la_value  0x%016lu", la_to_global, la_value);
    dbgprint("0x%016lx -> ca_value  0x%016lu", ca_to_global, ca_value);

    // Expect theat the RIP-relative load has substituted in gsrip fixed bits
    ASSERT_EQ(gsrip_loaded_value, ca_value);
}

TEST(ShadowRip, mem_store) {
    auto la_to_global = cc_dec_if_encoded_ptr(&g_value);
    auto ca_to_global = gen_cag_for(&g_value);
    const auto gsrip = ca_to_global;

    // Make sure g_value works as epxted
    store_g_value(1234);
    ASSERT_EQ(1234, load_g_value());

    // Enable gsrip and write value
    set_and_enable_shadow_rip(gsrip);
    store_g_value(1234);
    set_cpu_shadow_rip_enabled(false);

    // Expect we wrote as gsripped value and get garbage here!
    ASSERT_NE(1234, load_g_value());
}

TEST(ShadowRip, mem_copy_store_only) {
    auto gsrip = gen_cag_for(&g_value);

    // Make sure g_value works as epxted
    g_src = 1234;
    copy_src_to_g_value();
    ASSERT_EQ(1234, g_value);

    // Enable gsrip and write value
    set_and_enable_shadow_rip(gsrip);
    copy_src_to_g_value();
    set_cpu_shadow_rip_enabled(false);

    // Now expect we've got a garbled g_value!""
    ASSERT_NE(1234, g_value);
    ASSERT_EQ(1234, g_src);

    // Check we're back to prior behavior
    copy_src_to_g_value();
    ASSERT_EQ(1234, g_value);
}

TEST(ShadowRip, mem_copy_load_only) {
    auto gsrip = gen_cag_for(&g_src);

    // Make sure g_value works as epxted
    g_src = 1234;
    copy_src_to_g_value();
    ASSERT_EQ(1234, load_g_value());

    // Enable gsrip and write value
    set_and_enable_shadow_rip(gsrip);
    copy_src_to_g_value();
    set_cpu_shadow_rip_enabled(false);

    // Now expect we've got a garbled g_value!""
    ASSERT_NE(1234, g_value);
    ASSERT_EQ(1234, g_src);

    // Check we're back to prior behavior
    copy_src_to_g_value();
    ASSERT_EQ(1234, g_value);
}

TEST(ShadowRip, mem_copy) {
    auto gsrip = gen_cag_for(&g_src);

    // Make sure g_value works as expected
    g_src = 1234;
    copy_src_to_g_value();
    ASSERT_EQ(1234, load_g_value());

    // Enable gsrip and write value
    // NOTE: Need to set gsrip after every call because RET will reset due to
    //       the artificial test setup with "fake" gsrip values
    set_and_enable_shadow_rip(gsrip);
    // MAGIC(0);
    copy_src_to_g_value();  // Copy value stored before gsrip
    set_and_enable_shadow_rip(gsrip);
    auto bad_load = load_g_value();  // Store garbled loaded value
    set_and_enable_shadow_rip(gsrip);
    g_src = 1234;  // write to gsripped g_src
    set_and_enable_shadow_rip(gsrip);
    copy_src_to_g_value();  // copy from g_src to g_value
    set_and_enable_shadow_rip(gsrip);
    auto good_load = load_g_value();  // Store loaded value while gsripped
    set_cpu_shadow_rip_enabled(false);

    // Now expect we've got a garbled g_value
    ASSERT_NE(1234, bad_load);  // Expect garbled val on plaintext gsrip load
    ASSERT_NE(1234, g_src);     // Expect the gsrip src to store ciphertext
    ASSERT_NE(1234, g_value);   // Expect the gsrip copy to store ciphertext

    ASSERT_EQ(1234, good_load);  // Expect the gsripped load to be fine
    ASSERT_NE(g_src, g_value);   // But the ciphertext to differ
}

TEST(ShadowRip, mem_cmov) {
    auto gsrip = gen_cag_for(&g_src);

    g_value = 1234;

    // Enable gsrip and write value
    set_and_enable_shadow_rip(gsrip);
    auto cmov_loaded = do_cmov(5678);
    set_cpu_shadow_rip_enabled(false);

    ASSERT_NE(g_value, cmov_loaded);

    ASSERT_EQ(do_asm_cmov(false, 1234), 1234);
    ASSERT_NE(do_asm_cmov(true, 1234), 1234);
}

TEST(ShadowRip, mem_asm_cmov) {
    auto gsrip = (uint64_t)gen_cag_for((void *)&do_asm_cmov);

    set_and_enable_shadow_rip(gsrip);
    auto cmov_loaded = do_asm_cmov(true, 1234);
    set_cpu_shadow_rip_enabled(false);

    ASSERT_NE(cmov_loaded, do_asm_cmov(true, 1234));
}

__attribute__((noinline)) uint64_t mem_asm_push_func(uint64_t gsrip,
                                                     uint64_t val) {
    // First push value while shadow_rip is enabled
    set_and_enable_shadow_rip(gsrip);
    asm volatile("push    0x10(%%rip)  \n\t"
                 :
                 : [val] "r"(val)
                 : "rsp", "memory");

    // Then disable gsrip and pop the value back
    set_cpu_shadow_rip_enabled(false);
    asm volatile("pop    %[val]        \n\t"
                 : [val] "=r"(val)
                 :
                 : "rsp", "memory");

    return val;
}

TEST(ShadowRip, mem_asm_push) {
    // Use separate function so we can easily figure out its address for CA
    auto gsrip = (uint64_t)gen_cag_for((void *)&mem_asm_push_func);

    // First do a call with gsrip set
    uint64_t res1 = mem_asm_push_func(gsrip, 123);

    // Then do a call without gsrip set
    uint64_t res2 = mem_asm_push_func(0, 123);

    ASSERT_NE(res1, res2);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    set_cpu_shadow_rip_enabled(false);
    return RUN_ALL_TESTS();
}
