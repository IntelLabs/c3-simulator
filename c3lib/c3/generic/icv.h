// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef C3LIB_C3_GENERIC_ICV_H_
#define C3LIB_C3_GENERIC_ICV_H_

#include "c3/generic/c3_conf.h"
#include "c3/generic/defines.h"

#ifdef CC_INTEGRITY_ENABLE

/**
 * @brief Size of ICV slot/granule
 */
#define SLOT_SIZE_IN_BITS 3
#define SLOT_SIZE_IN_BYTES (((uintptr_t)1) << SLOT_SIZE_IN_BITS)

/**
 * @brief Mask to get ICV slot base address
 */
#define ICV_ADDR_MASK 0xFFFFFFFFFFFFFFF8

/**
 * @brief Mask to get INIT bit from the ICV
 */
#define ICV_INIT_MASK 0x0000000000000002

/**
 * @brief Mask to get PREINIT bit from the ICV
 */
#define ICV_PREINIT_MASK 0x0000000000000004

/*
 * Macros to set and get the initialization state within an ICV
 *
 * Notice how it the polarity is inverted:
 *   0 = initialized
 *   1 = uninitilized
 */
/**
 * @brief Macro to set initialization state to INITIALIZED
 */
#define ICV_INITIALIZE(x) (x & (~ICV_INIT_MASK))
/**
 * @brief Macro to set initialization state to UNINITIALIZED
 */
#define ICV_UNINITIALIZE(x) (x | ICV_INIT_MASK)
/**
 * @brief Yields TRUE if the ICV has been initialized
 */
#define IS_ICV_INITIALIZED(x) (((x) & (ICV_INIT_MASK)) == 0)
/**
 * @brief Yields TRUE if the ICV has not been initialized
 */
#define IS_ICV_UNINITIALIZED(x) (((x) & (ICV_INIT_MASK)) == ICV_INIT_MASK)

/*
 * Macros to set and get PREINIT enabled bit
 *
 * When PREINIT is enabled, writes to a memory granule automatically promote it
 * to the INITIALIZED state. Conversely, when it is disabled, a granule can
 * only migrate from the UNINITIALIZED to the INITIALIZED state with an
 * explicit InitICV instruction.
 */
/**
 * @brief Yields TRUE if ICV has PREINIT semantics enabled; FALSE when disabled
 */
#define IS_ICV_PREINIT_ENABLED(x)                                              \
    (((x) & (ICV_PREINIT_MASK)) == ICV_PREINIT_MASK)
/**
 * @brief Macro to enable PREINIT semantics
 */
#define ICV_PREINIT_ENABLE(x) (x | ICV_PREINIT_MASK)
/**
 * @brief Macro to disable PREINIT semantics
 */
#define ICV_PREINIT_DISABLE(x) (x & (~ICV_PREINIT_MASK))

/**
 * @brief Fixed size used to iniialize internal BIOS map
 */
#define BIOS_SIZE 0x200000000

/**
 * @brief The data type of ICV
 */
typedef uint64_t icv_t;

static inline void cc_isa_invicv(uint64_t pointer);
static inline void cc_isa_initicv(const uint64_t ptr, const uint64_t data);
static inline void cc_isa_preiniticv(const uint64_t ptr);

/**
 * @brief Retrieve ICV locked state from C3 context data
 */
static inline int cc_ctx_get_icv_lock(const struct cc_context *ctx) {
    return ctx->flags_.bitfield_.rsvd3_;
}

/**
 * @brief Set ICV locked state to C3 context data
 */
static inline void cc_ctx_set_icv_lock(struct cc_context *ctx, int val) {
    ctx->flags_.bitfield_.rsvd3_ = val;
}

/**
 * @brief Retrieve ICV enabled state from C3 context data
 */
static inline int cc_ctx_get_icv_enabled(const struct cc_context *ctx) {
    return ctx->flags_.bitfield_.rsvd4_;
}

/**
 * @brief Set ICV enabled state to C3 context data
 */
static inline void cc_ctx_set_icv_enabled(struct cc_context *ctx, int val) {
    ctx->flags_.bitfield_.rsvd4_ = val;
}

static inline int cc_ctx_get_icv_map_reset(const struct cc_context *ctx) {
    return ctx->flags_.bitfield_.rsvd6_;
}

static inline void cc_ctx_set_icv_map_reset(struct cc_context *ctx, int val) {
    ctx->flags_.bitfield_.rsvd6_ = val;
}

/**
 * @brief Configures C3 ICV lock option
 *
 * This is currently mainly a workaround used by the set ICV functionality until
 * we implement proper SetICV ISA emulation such that those can directly use a
 * single instruction. This also currently works in user-space since the C3
 * configuration ISA this uses isn't restricted at all (which it would be in a
 * real setting).
 */
static inline void cc_set_icv_lock(int val) {
    struct cc_context ctx = {0};
    cc_save_context(&ctx);
    cc_ctx_set_icv_lock(&ctx, val);
    cc_load_context(&ctx);
}

static inline void cc_set_icv_enabled(int val) {
    struct cc_context ctx = {0};
    cc_save_context(&ctx);
    cc_ctx_set_icv_enabled(&ctx, val);
    cc_load_context(&ctx);
}

/**
 * @brief Can be used on target to trigger ICV reset
 *
 * NOTE: This is a temporary workaround that allows cleaning of ICVs that are
 * currently mapped to virtual addresses system-wide (i.e., not to specific
 * process)
 */
static inline void cc_trigger_icv_map_reset(void) {
    struct cc_context ctx;
    cc_save_context(&ctx);
    cc_ctx_set_icv_map_reset(&ctx, 1);
    cc_load_context(&ctx);
}

/**
 * @brief Sets ICV for a memory range and zeroes memory
 */
static inline void cc_set_icv_and_zero(void *addr, size_t size) {
    char *uptr = (char *)addr;  // NOLINT
    char *const end = uptr + size;
    cc_set_icv_lock(0);
    for (; uptr < end; uptr += sizeof(char)) {
        *uptr = 0;
    }
    cc_set_icv_lock(1);
}

/**
 * @brief Sets ICV for a memory range but keeps memory content
 */
static inline void cc_set_icv(void *addr, size_t size) {
    char *uptr = (char *)addr;  // NOLINT
    char *const end = uptr + size;
    cc_set_icv_lock(0);
    for (; uptr < end; uptr += sizeof(char)) {
        __asm__ volatile("mov (%[ptr]), %%al  \n"
                         "mov %%al, (%[ptr])  \n"
                         :
                         : [ptr] "r"(uptr)
                         : "memory", "rax");
    }
    cc_set_icv_lock(1);
}

static inline void *cc_icv_memcpy(void *dst, const void *src, size_t n) {
    void *res = dst;
    asm volatile(".byte 0x3e                     \n"
                 ".byte 0xf3                     \n"
                 ".byte 0xa4                     \n"
                 : "+D"(dst), "+S"(src), "+c"(n)::"memory");
    return res;
}

static inline void *cc_icv_memcpy_permissive(void *dst, const void *src,
                                             size_t n) {
    void *res = dst;
    asm volatile(".byte 0x26                     \n"
                 ".byte 0xf3                     \n"
                 ".byte 0xa4                     \n"
                 : "+D"(dst), "+S"(src), "+c"(n)::"memory");
    return res;
}

static inline void *cc_icv_memmove(void *dst, const void *src, size_t n) {
    void *res = dst;
    if (dst > src) {
        const char *src_end = (const char *)src + n - 1;
        char *dst_end = (char *)dst + n - 1;  // NOLINT
        asm volatile("std                            \n"
                     ".byte 0x3e                     \n"
                     ".byte 0xf3                     \n"
                     ".byte 0xa4                     \n"
                     "cld                            \n"
                     : "+D"(dst_end), "+S"(src_end), "+c"(n)::"memory");
    } else {
        asm volatile(".byte 0x3e                     \n"
                     ".byte 0xf3                     \n"
                     ".byte 0xa4                     \n"
                     : "+D"(dst), "+S"(src), "+c"(n)::"memory");
    }
    return res;
}

static inline void *cc_icv_memmove_permissive(void *dst, const void *src,
                                              size_t n) {
    void *res = dst;
    if (dst > src) {
        const char *src_end = (const char *)src + n - 1;
        char *dst_end = (char *)dst + n - 1;  // NOLINT
        asm volatile("std                            \n"
                     ".byte 0x26                     \n"
                     ".byte 0xf3                     \n"
                     ".byte 0xa4                     \n"
                     "cld                            \n"
                     : "+D"(dst_end), "+S"(src_end), "+c"(n)::"memory");
    } else {
        asm volatile(".byte 0x26                     \n"
                     ".byte 0xf3                     \n"
                     ".byte 0xa4                     \n"
                     : "+D"(dst), "+S"(src), "+c"(n)::"memory");
    }
    return res;
}

/// Setup ICV for allocation
static inline void *cc_setup_icv_for_alloc(void *p_encoded, size_t size,
                                           size_t usable_size, int zero) {
#ifdef CC_DETECT_1B_OVF
    // Adjust p_encoded so that the end of the region of the original requested
    // size aligns with the end of the final initialized ICV granule.
    uintptr_t p_encoded_int = (uintptr_t)p_encoded;
    uintptr_t p_encoded_end = p_encoded_int + size;
    uintptr_t p_encoded_aligned_end = p_encoded_end;
    if (size % SLOT_SIZE_IN_BYTES != 0) {
        p_encoded_aligned_end &= ICV_ADDR_MASK;
        p_encoded_aligned_end += SLOT_SIZE_IN_BYTES;
    }
    p_encoded_int += p_encoded_aligned_end - p_encoded_end;
    p_encoded = (void *)p_encoded_int;  // NOLINT

    // Just set ICVs for the requested size of the allocation, not the full
    // usable size. This may lead to incompatibilities with certain workloads
    // that expect to be able to access the full usable size, but setting
    // additional ICVs beyond the requested size prevents detecting some small
    // overflows.
    usable_size = size;
#endif

    if (zero) {
        cc_set_icv_and_zero(p_encoded, usable_size);
    } else {
        cc_set_icv(p_encoded, usable_size);
    }
    return p_encoded;
}

/*******************************************************************************
 * C++ specific helper functions
 ******************************************************************************/
#if defined(__cplusplus)

/**
 * @brief Call C3 ISA to invalidate given pointer using provided metadata
 *
 * @tparam T type convertible to a uint64_t
 * @param ptr
 * @return void
 */
template <typename T> static inline void cc_isa_invicv(T ptr) {
    cc_isa_invicv((uint64_t)ptr);
}

template <typename P, typename D>
static inline void cc_isa_initicv(const P ptr, const D data) {
    cc_isa_initicv((const uint64_t)ptr, (const uint64_t)data);
}

template <typename P> static inline void cc_isa_preiniticv(P ptr) {
    cc_isa_preiniticv((const uint64_t)ptr);
}

#endif  // defined(__cplusplus)

#endif  // CC_INTEGRITY_ENABLE

#ifdef C3_X86_64
#include "c3/x86_64/x86_64.icv.h"
#endif  // C3_X86_64

#endif  // C3LIB_C3_GENERIC_ICV_H_
