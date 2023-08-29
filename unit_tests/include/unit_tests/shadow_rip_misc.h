#ifndef UNIT_TESTS_INCLUDE_UNIT_TESTS_SHADOW_RIP_MISC_H_
#define UNIT_TESTS_INCLUDE_UNIT_TESTS_SHADOW_RIP_MISC_H_

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
