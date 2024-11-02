// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef C3LIB_C3_GENERIC_CA_H_
#define C3LIB_C3_GENERIC_CA_H_

#include "c3/generic/defines.h"

// Define CA format
#define MIN_ALLOC_OFFSET 0
#define VERSION_SIZE 9

#define TOP_CANONICAL_BIT_OFFSET 47
#define PLAINTEXT_SIZE 32
#define CIPHERTEXT_SIZE 24
#define SIZE_SIZE 6
#define CIPHERTEXT_LOW_SIZE 15
#define S_BIT_SIZE 1
#define CIPHERTEXT_HIGH_OFFSET                                                 \
    (PLAINTEXT_SIZE + CIPHERTEXT_LOW_SIZE + S_BIT_SIZE)
#define S_EXTENDED_SIZE                                                        \
    (64 - 2 * S_BIT_SIZE - CIPHERTEXT_LOW_SIZE - PLAINTEXT_SIZE)
#define SPECIAL_SIZE_ENCODING_WITH_ADJUST 31

#define PLAINTEXT_OFFSET 0
#define CIPHERTEXT_OFFSET (PLAINTEXT_OFFSET + PLAINTEXT_SIZE)
#define VERSION_OFFSET (CIPHERTEXT_OFFSET + CIPHERTEXT_SIZE - VERSION_SIZE)
#define SIZE_OFFSET (CIPHERTEXT_OFFSET + CIPHERTEXT_SIZE)

#define C3_FMASK 0xFFFFFFFFFFFFFFFFULL

#define CC_STACK_ID_VAL 0x3e

typedef struct ptr_metadata {
    union {
        uint64_t uint64_;
        struct {
            uint64_t size_ : SIZE_SIZE;
            uint64_t version_ : VERSION_SIZE;
            uint64_t adjust_ : 1;
            uint64_t unused_ : 64 - (SIZE_SIZE + VERSION_SIZE + 1);
        };
    };
} ptr_metadata_t;

typedef struct {
    union {
        uint64_t uint64_;
        struct {
            uint64_t plaintext_ : PLAINTEXT_SIZE;
            uint64_t ciphertext_low_ : CIPHERTEXT_LOW_SIZE;
            uint64_t s_prime_bit_ : S_BIT_SIZE;
            uint64_t ciphertext_high_ : CIPHERTEXT_SIZE - CIPHERTEXT_LOW_SIZE;
            uint64_t enc_size_ : SIZE_SIZE;
            uint64_t s_bit_ : S_BIT_SIZE;
        };
        struct {
            uint64_t : PLAINTEXT_SIZE;
            uint64_t plaintext_low_ : CIPHERTEXT_LOW_SIZE;
            uint64_t : S_BIT_SIZE;
            uint64_t s_extended_ : S_EXTENDED_SIZE;
            uint64_t : S_BIT_SIZE;
        };
        struct {
            uint64_t : 64 - (VERSION_SIZE + SIZE_SIZE + S_BIT_SIZE);
#if (VERSION_SIZE > 0)
            uint64_t version_ : VERSION_SIZE;
#endif  // (VERSION_ZIE > 0)
            uint64_t : SIZE_SIZE;
            uint64_t : S_BIT_SIZE;
        };
    };
} ca_t;

typedef ca_t canonical_pointer_t;
typedef ca_t cc_pointer_t;

static inline ca_t get_ca_t(uint64_t ptr) {
    ca_t ca;
    ca.uint64_ = ptr;
    return ca;
}

static inline uint64_t get_low_canonical_bits(uint64_t pointer) {
    return pointer & (C3_FMASK >> (64 - TOP_CANONICAL_BIT_OFFSET - 1));
}

static inline cc_pointer_t *convert_to_cc_ptr(uint64_t *pointer) {
    return (cc_pointer_t *)pointer;  // NOLINT
}

static inline uint8_t ca_get_size(uint64_t pointer) {
    return convert_to_cc_ptr(&pointer)->enc_size_;
}

static inline uint8_t ca_get_adjust(uint64_t pointer) {
    uint8_t enc_size = ca_get_size(pointer);
    return (enc_size == SPECIAL_SIZE_ENCODING_WITH_ADJUST) ? 1 : 0;
}

static inline uint64_t get_ciphertext(uint64_t pointer) {
    uint64_t ciphertext =
            ((uint64_t)convert_to_cc_ptr(&pointer)->ciphertext_high_)
            << CIPHERTEXT_LOW_SIZE;
    ciphertext |= (uint64_t)convert_to_cc_ptr(&pointer)->ciphertext_low_;
    return ciphertext;
}

static inline uint64_t get_plaintext(uint64_t pointer) {
    return (uint64_t)convert_to_cc_ptr(&pointer)->plaintext_;
}

static inline uint64_t get_tweak_mask(uint8_t size) {
    return (C3_FMASK << (size + MIN_ALLOC_OFFSET - 1));
}

static inline int is_canonical(uint64_t pointer) {
    const uint64_t high = pointer >> (TOP_CANONICAL_BIT_OFFSET);
    return high == 0 || high == (C3_FMASK >> (TOP_CANONICAL_BIT_OFFSET));
}

static inline size_t decode_size(uint8_t size_encoded) {
    return (size_t)(                                                 // NOLINT
            1ULL << ((size_t)size_encoded + MIN_ALLOC_OFFSET - 1));  // NOLINT
}

static inline size_t get_size_in_bytes(uint64_t pointer) {
    uint8_t size_encoded = ca_get_size(pointer);
    return decode_size(size_encoded);
}

/**
 * @brief Get the tweak bits from a given CA
 */
static inline uint64_t ca_get_tweak_bits_u64(uint64_t ptr) {
    return ptr & get_tweak_mask(get_ca_t(ptr).enc_size_);
}

/**
 * @brief Get largest possible offset given input pointer CA power-slot
 *
 * Note that this only checks the bounds in terms of CA power-slots, which may
 * or may not match the underlying in-memory object's bounds.
 */
static inline size_t ca_get_inbound_offset(const void *ptr, const size_t i) {
    const uint64_t u64_ptr = (uint64_t)((uintptr_t)ptr);
    const uint64_t mask = ~get_tweak_mask(get_ca_t(u64_ptr).enc_size_);
    const uint64_t max = mask - (mask & u64_ptr);
    return (i < max ? i : max);
}

static inline uint64_t mask_n_lower_bits(uint64_t val, int n) {
    return ((val >> n) << n);
}

/**
 * @brief Check if ptr is any type of C3 encoded pointer
 *
 * @param ptr
 * @return int
 */
static inline int is_encoded_cc_ptr(const uint64_t ptr) {
    const ca_t ca = get_ca_t(ptr);
    return (ca.enc_size_ == 0x0 || ca.enc_size_ == C3_BITMASK(SIZE_SIZE)) ? 0
                                                                          : 1;
}

/**
 * @brief Check if ptr is C3 encoded stack pointer
 *
 * @param ptr
 * @return int 0 if not a C3 stack pointer, otherwise non-zero
 */
static inline int is_encoded_cc_stack_ptr(const uint64_t ptr) {
    const ca_t ca = get_ca_t(ptr);
    return is_encoded_cc_ptr(ca.uint64_) && ca.enc_size_ == CC_STACK_ID_VAL;
}

/**
 * @brief Check if ptr is C3 encoded heap pointer
 *
 * @param ptr
 * @return int 0 if not a C3 heap pointer, otherwise non-zero
 */
static inline int is_encoded_cc_heap_ptr(const uint64_t ptr) {
    const ca_t ca = get_ca_t(ptr);
    return is_encoded_cc_ptr(ca.uint64_) && ca.enc_size_ != CC_STACK_ID_VAL;
}

/**
 * @brief Encode CA using C3 ISA
 */
static inline uint64_t cc_isa_encptr(uint64_t ptr, const ptr_metadata_t *md);

/**
 * @brief Decode CA using C3 ISA
 */
static inline uint64_t cc_isa_decptr(uint64_t pointer);

static inline uint64_t cc_dec_if_encoded_ptr(uint64_t ptr) {
    return is_encoded_cc_ptr(ptr) ? cc_isa_decptr(ptr) : ptr;
}

#define ca_get_tweak_bits(ptr) (ca_get_tweak_bits_u64((uint64_t)ptr))

/*******************************************************************************
 * C++ specific helper functions
 ******************************************************************************/
#if defined(__cplusplus)

template <typename T> static inline ca_t get_ca_t(T ptr) {
    ca_t ca;
    ca.uint64_ = (uint64_t)ptr;
    return ca;
}

static inline ca_t get_ca_t(ca_t ptr) { return ptr; }

#undef ca_get_tweak_bits  // Replace C-only macro with C++ template function
template <typename T> static inline uint64_t ca_get_tweak_bits(T ptr) {
    return ca_get_tweak_bits_u64(reinterpret_cast<uint64_t>(ptr));
}

/**
 * @brief Check if given pointer is a CA
 */
template <typename T> static inline bool is_encoded_cc_ptr(const T ptr) {
    return is_encoded_cc_ptr((const uint64_t)ptr);
}

static inline bool is_encoded_cc_ptr(const ca_t ptr) {
    return is_encoded_cc_ptr(ptr.uint64_);
}

/**
 * @brief Check if given pointer is a C3 encoded stack pointer
 */
template <typename T>
static inline bool is_encoded_cc_stack_ptr(const uint64_t ptr) {
    return is_encoded_cc_stack_ptr((const uint64_t)ptr);
}

/**
 * @brief Check if given pointer is a C3 encoded heap pointer
 */
template <typename T>
static inline bool is_encoded_cc_heap_ptr(const uint64_t ptr) {
    return is_encoded_cc_heap_ptr((const uint64_t)ptr);
}

/**
 * @brief Call C3 ISA to decode given pointer
 *
 * @tparam T type convertible to a uint64_t
 * @param ptr
 * @return T
 */
template <typename T> static inline T cc_isa_decptr(T ptr) {
    return (T)cc_isa_decptr((uint64_t)ptr);
}

/**
 * @brief Call C3 ISA to encode given pointer using provided metadata
 *
 * @tparam T type convertible to a uint64_t
 * @param ptr
 * @param md Metadata passed as given to C3 ISA
 * @return T
 */
template <typename T>
static inline T cc_isa_encptr(T ptr, const ptr_metadata_t *md) {
    return (T)cc_isa_encptr((uint64_t)ptr, md);
}

#ifndef _CC_GLOBALS_NO_INCLUDES_

// Defined in try_box.h
static inline uint64_t cc_isa_encptr_sv(uint64_t p, size_t s, uint8_t v);

/**
 * @brief Call C3 ISA after preparing metadata based on function arguments.
 *
 * @tparam T type convertible to a uint64_t
 * @param pointer
 * @param size Size in bytes, will be fitted to power-of-2 slot
 * @param version Optional version to embed in CA (default: 0)
 * @return T
 */
template <typename T>
static inline T cc_isa_encptr(T ptr, size_t size, uint8_t version = 0) {
    return (T)cc_isa_encptr_sv((uint64_t)ptr, size, version);
}
#endif  // _CC_GLOBALS_NO_INCLUDES_

/**
 * @brief Check if ptr is encoded, and if so, decode it
 *
 * @tparam T type convertible to a uint64_t
 * @param ptr
 * @return T
 */
template <typename T> static inline T cc_dec_if_encoded_ptr(T ptr) {
    return is_encoded_cc_ptr(ptr) ? cc_isa_decptr(ptr) : ptr;
}

template <typename T> static inline uint64_t get_ciphertext(T ptr) {
    return get_ciphertext(reinterpret_cast<uint64_t>(ptr));
}

#endif  // defined(__cplusplus)

#ifndef _CC_GLOBALS_NO_INCLUDES_
#include "c3/malloc/try_box.h"  // NOLINT (NOTE: Depend on defines above)
#endif

#ifdef C3_X86_64
#include "c3/x86_64/x86_64.ca.h"
#endif  // C3_X86_64
#ifdef C3_RISC_V
#include "c3/risc-v/risc-v.ca.h"
#endif  // C3_RISC_V

#endif  // C3LIB_C3_GENERIC_CA_H_
