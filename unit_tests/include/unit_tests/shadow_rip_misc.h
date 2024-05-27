#ifndef UNIT_TESTS_INCLUDE_UNIT_TESTS_SHADOW_RIP_MISC_H_
#define UNIT_TESTS_INCLUDE_UNIT_TESTS_SHADOW_RIP_MISC_H_

#include "crypto/cc_encoding.h"
#include "malloc/cc_globals.h"

static inline void set_cpu_shadow_rip_enabled(bool shadow_rip_enabled) {
    struct cc_context ctx;
    cc_save_context(&ctx);
    cc_ctx_set_shadow_rip_enabled(&ctx, shadow_rip_enabled);
    cc_load_context(&ctx);
}

template <typename T> static inline void set_cpu_shadow_rip(T shadow_rip) {
    struct cc_context ctx;
    cc_save_context(&ctx);
    cc_ctx_set_shadow_rip(&ctx, shadow_rip);
    cc_load_context(&ctx);
}

template <typename T>
static inline void set_and_enable_shadow_rip(T shadow_rip) {
    struct cc_context ctx;
    cc_save_context(&ctx);
    cc_ctx_set_shadow_rip(&ctx, shadow_rip);
    cc_ctx_set_shadow_rip_enabled(&ctx, true);
    cc_load_context(&ctx);
}

template <typename T> static inline void *gen_gsrip(T ptr, size_t size) {
    return cc_isa_encptr(cc_dec_if_encoded_ptr(ptr), size);
}

template <typename T, typename I> static inline T gen_cag(T ptr, I b, I e) {
    const auto size = ((uint64_t)e - (uint64_t)b);
    auto base = cc_isa_encptr(cc_dec_if_encoded_ptr(b), size);

    // Generate mask for mutable pointer bits
    const auto ptr_md = get_pointer_metadata((uint64_t)base);
    const auto mask = get_tweak_mask(ptr_md.size_);

    const auto gsrip_bits = mask & (uint64_t)base;
    const auto offset_bits = ~mask & (uint64_t)ptr;
    return (T)(gsrip_bits | offset_bits);
}

template <typename T>
static inline T gen_gsrip_range(T _ptr1, T _ptr2, size_t size) {
    auto ptr1 = reinterpret_cast<uint64_t>(_ptr1);
    auto ptr2 = reinterpret_cast<uint64_t>(_ptr2);
    ptr1 = cc_dec_if_encoded_ptr(ptr1);
    ptr2 = cc_dec_if_encoded_ptr(ptr2);

    auto min = ptr1 < ptr2 ? ptr1 : ptr2;
    auto max = ptr1 > ptr2 ? ptr1 : ptr2;
    size += (max - min);
    return reinterpret_cast<T>(cc_isa_encptr(min, size));
}

#endif  // UNIT_TESTS_INCLUDE_UNIT_TESTS_SHADOW_RIP_MISC_H_
