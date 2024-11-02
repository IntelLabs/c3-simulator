// Copyright 2016-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef C3LIB_C3_GENERIC_C3_CONF_H_
#define C3LIB_C3_GENERIC_C3_CONF_H_

#include "c3/generic/ciphers.h"
#include "c3/generic/defines.h"

#define CTX_UNUSED_BITS 57

#ifdef CC_SHADOW_RIP_ENABLE
// We need some extra bits for shadow RIP
#undef CTX_UNUSED_BITS
#define CTX_UNUSED_BITS 56
#endif  // CC_SHADOW_RIP_ENABLE

typedef struct __attribute__((__packed__)) cc_context {
    union {
        struct {
            uint64_t unused_ : CTX_UNUSED_BITS;
#ifdef CC_SHADOW_RIP_ENABLE
            uint64_t shadow_rip_enabled_ : 1;
#endif  // CC_SHADOW_RIP_ENABLE
            uint64_t rsvd6_ : 1;
            uint64_t rsvd5_ : 1;
            uint64_t rsvd4_ : 1;
            uint64_t rsvd3_ : 1;
            uint64_t rsvd2_ : 1;
            uint64_t rsvd1_ : 1;
            uint64_t cc_enabled_ : 1;
        } bitfield_;
        uint64_t raw_;
    } flags_;  // 64-bit flags field
#ifdef CC_SHADOW_RIP_ENABLE
    uint64_t shadow_rip_;
#endif  // CC_SHADOW_RIP_ENABLE
    uint64_t reserved1_;
    data_key_bytes_t ds_key_bytes_;       // data key shared
    data_key_bytes_t dp_key_bytes_;       // data key private
    data_key_bytes_t c_key_bytes_;        // code key
    pointer_key_bytes_t addr_key_bytes_;  // address / pointer key
} cc_context_t;

/**
 * @brief Saves current C3 state into given cc_context buffer
 *
 * @param ptr
 */
static inline void cc_save_context(struct cc_context *ptr);

/**
 * @brief Loads C3 state from given cc_context buffer
 *
 * @param ptr
 */
static inline void cc_load_context(const struct cc_context *ptr);

static inline int cc_ctx_get_cc_enabled(const struct cc_context *ctx) {
    return ctx->flags_.bitfield_.cc_enabled_;
}

static inline void cc_ctx_set_cc_enabled(struct cc_context *ctx, int val) {
    ctx->flags_.bitfield_.cc_enabled_ = val;
}

#ifdef CC_SHADOW_RIP_ENABLE

static inline void cc_ctx_set_shadow_rip_enabled(struct cc_context *ctx,
                                                 int shadow_rip_enabled) {
    ctx->flags_.bitfield_.rsvd5_ = (shadow_rip_enabled != 0);
}

static inline int cc_ctx_get_shadow_rip_enabled(const struct cc_context *ctx) {
    return ctx->flags_.bitfield_.rsvd5_ == 1;
}

#if defined(__cplusplus)
template <typename T>
static inline void cc_ctx_set_shadow_rip(struct cc_context *ctx,
                                         const T shadow_rip) {
    ctx->shadow_rip_ = reinterpret_cast<uint64_t>(shadow_rip);
}
#endif  // defined(__cplusplus)

#if defined(__cplusplus)
template <typename T>
static inline T cc_ctx_get_shadow_rip(const struct cc_context *ctx) {
    return static_cast<T>(ctx->shadow_rip_);
}
#endif  // defined(__cplusplus)

#endif  // CC_SHADOW_RIP_ENABLE

#ifdef C3_X86_64
#include "c3/x86_64/x86_64.c3_conf.h"
#endif  // C3_X86_64
#ifdef C3_RISC_V
#include "c3/risc-v/risc-v.c3_conf.h"
#endif  // C3_X86_64RISC_V

#endif  // C3LIB_C3_GENERIC_C3_CONF_H_
