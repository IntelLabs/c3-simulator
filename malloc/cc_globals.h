/*
 Copyright 2016 Intel Corporation
 SPDX-License-Identifier: MIT
*/
/*
Contains global defines shared between malloc and Simics models
*/

#ifndef MALLOC_CC_GLOBALS_H_
#define MALLOC_CC_GLOBALS_H_
#ifndef _CC_GLOBALS_NO_INCLUDES_
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#endif  // !_CC_GLOBALS_NO_INCLUDES_

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

// Ciphers
#define BIPBIP_CIPHER 1
#define ASCON_CIPHER 2
// #define K_CIPHER64 2

#ifndef MAGIC
#define MAGIC(n)                                                               \
    do {                                                                       \
        int simics_magic_instr_dummy;                                          \
        __asm__ __volatile__("cpuid"                                           \
                             : "=a"(simics_magic_instr_dummy)                  \
                             : "a"(0x4711 | ((unsigned)(n) << 16))             \
                             : "ecx", "edx", "ebx");                           \
    } while (0)
#endif

#define BITMASK(x) (0xFFFFFFFFFFFFFFFF >> (64 - (x)))

#define MAGIC_LIT_START 1
#define MAGIC_LIT_END 2
#define MAGIC_MALLOC_ENTER 3
#define MAGIC_MALLOC_EXIT 4
#define MAGIC_CALLOC_ENTER 5
#define MAGIC_CALLOC_EXIT 6
#define MAGIC_REALLOC_ENTER 7
#define MAGIC_REALLOC_EXIT 8
#define MAGIC_FREE_ENTER 9
#define MAGIC_FREE_EXIT 10

#define CC_STACK_ID_VAL 0x3e

#define KEY_SIZE(cipher)                                                       \
    (((cipher) == BIPBIP_CIPHER) ? 32 : ((cipher) == ASCON_CIPHER) ? 16 : -1)

#define KEY_SCHEDULE_SIZE(cipher) (((cipher) == ASCON_CIPHER) ? 16 : -1)

#ifndef CC_POINTER_CIPHER
#define CC_POINTER_CIPHER BIPBIP_CIPHER
#endif

#ifndef CC_DATA_CIPHER
#define CC_DATA_CIPHER ASCON_CIPHER
#endif

#define CIPHER_OFFSET_BITS 3

typedef uint8_t pointer_key_bytes_t[KEY_SIZE(CC_POINTER_CIPHER)];
typedef struct {
    int size_;
    uint8_t bytes_[KEY_SIZE(CC_POINTER_CIPHER)];
} pointer_key_t;

typedef uint8_t data_key_bytes_t[KEY_SIZE(CC_DATA_CIPHER)];
typedef uint8_t data_key_schedule_t[KEY_SCHEDULE_SIZE(CC_DATA_CIPHER)];
typedef struct {
    int size_;
    uint8_t bytes_[KEY_SIZE(CC_DATA_CIPHER)];
} data_key_t;





#define MIN_ALLOC_OFFSET 0
#define VERSION_SIZE 9

// Pointer format defines
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

#define FMASK 0xFFFFFFFFFFFFFFFFULL

#define BOX_PAD_FOR_STRLEN_FIX 48

// Use a prime number for depth for optimum results
#define QUARANTINE_DEPTH 373
#define QUARANTINE_WIDTH 2

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
        unsigned long long ull_;  // NOLINT
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

struct __attribute__((__packed__)) cc_context {
    union {
        struct {
            uint64_t unused_ : 61;
            uint64_t rsvd2_ : 1;
            uint64_t rsvd1_ : 1;
            uint64_t cc_enabled_ : 1;
        } bitfield_;
        uint64_t raw_;
    } flags_;  // 64-bit flags field
    uint64_t reserved1_;
    data_key_bytes_t ds_key_bytes_;       // data key shared
    data_key_bytes_t dp_key_bytes_;       // data key private
    data_key_bytes_t c_key_bytes_;        // code key
    pointer_key_bytes_t addr_key_bytes_;  // address / pointer key
};

typedef ca_t canonical_pointer_t;
typedef ca_t cc_pointer_t;

#ifndef _CC_GLOBALS_NO_INCLUDES_
#include "try_box.h"  // NOLINT (NOTE: This depend on the above defines)
#endif

static inline uint64_t get_low_canonical_bits(uint64_t pointer) {
    return pointer & (FMASK >> (64 - TOP_CANONICAL_BIT_OFFSET - 1));
}

static inline cc_pointer_t *convert_to_cc_ptr(uint64_t *pointer) {
    return (cc_pointer_t *)pointer;  // NOLINT
}

static inline uint8_t get_size(uint64_t pointer) {
    return convert_to_cc_ptr(&pointer)->enc_size_;
}

static inline uint8_t get_adjust(uint64_t pointer) {
    uint8_t enc_size = get_size(pointer);
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
    return (FMASK << (size + MIN_ALLOC_OFFSET - 1));
}

static inline int is_canonical(uint64_t pointer) {
    return (pointer == get_low_canonical_bits(pointer)) ? 1 : 0;
}

static inline size_t decode_size(uint8_t size_encoded) {
    return (size_t)(                                                 // NOLINT
            1ULL << ((size_t)size_encoded + MIN_ALLOC_OFFSET - 1));  // NOLINT
}

static inline size_t get_size_in_bytes(uint64_t pointer) {
    uint8_t size_encoded = get_size(pointer);
    return decode_size(size_encoded);
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
    const ca_t ca = {.uint64_ = ptr};
    return (ca.enc_size_ == 0x0 || ca.enc_size_ == BITMASK(SIZE_SIZE)) ? 0 : 1;
}

/**
 * @brief Check if ptr is C3 encoded stack pointer
 *
 * @param ptr
 * @return int 0 if not a C3 stack pointer, otherwise non-zero
 */
static inline int is_encoded_cc_stack_ptr(const uint64_t ptr) {
    const ca_t ca = {.uint64_ = ptr};
    return is_encoded_cc_ptr(ca.uint64_) && ca.enc_size_ == CC_STACK_ID_VAL;
}

/**
 * @brief Check if ptr is C3 encoded heap pointer
 *
 * @param ptr
 * @return int 0 if not a C3 heap pointer, otherwise non-zero
 */
static inline int is_encoded_cc_heap_ptr(const uint64_t ptr) {
    const ca_t ca = {.uint64_ = ptr};
    return is_encoded_cc_ptr(ca.uint64_) && ca.enc_size_ != CC_STACK_ID_VAL;
}

static inline uint64_t cc_isa_encptr(uint64_t ptr, const ptr_metadata_t *md) {
    asm(".byte 0xf0			\n"
        ".byte 0x48			\n"
        ".byte 0x01			\n"
        ".byte 0xc8         \n"
        : [ ptr ] "+a"(ptr)
        : [ md ] "c"((uint64_t)md->uint64_)
        :);
    return ptr;
}

static inline uint64_t cc_isa_decptr(uint64_t pointer) {
    asm(".byte 0xf0   		\n"
        ".byte 0x48			\n"
        ".byte 0x01			\n"
        ".byte 0xc0         \n"
        : [ ptr ] "+a"(pointer)
        :
        :);
    return pointer;
}

/**
 * @brief Saves current C3 state into given cc_context buffer
 *
 * @param ptr
 */
static inline void cc_save_context(struct cc_context *ptr) {
    __asm__ __volatile__(".byte 0xf0; .byte 0x2f" : : "a"(ptr));
}

/**
 * @brief Loads C3 state from given cc_context buffer
 *
 * @param ptr
 */
static inline void cc_load_context(const struct cc_context *ptr) {
    __asm__ __volatile__(".byte 0xf0; .byte 0xfa" : : "a"(ptr));
}

static inline uint64_t cc_dec_if_encoded_ptr(uint64_t ptr) {
    return is_encoded_cc_ptr(ptr) ? cc_isa_decptr(ptr) : ptr;
}

#if defined(__cplusplus)

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
    ptr_metadata_t md = {0};
    if (!try_box((uint64_t)ptr, size, &md)) {
#ifndef _CC_GLOBALS_NO_INCLUDES_
        assert(false && "Try box failed");
#endif  // _CC_GLOBALS_NO_INCLUDES_
        return (T) nullptr;
    }
    md.version_ = version;
    return (T)cc_isa_encptr(ptr, &md);
}

/**
 * @brief Determine if given pointer is a CA
 *
 * @tparam T type convertible to a uint64_t
 * @param ptr
 * @return true
 * @return false
 */
template <typename T> static inline bool is_encoded_cc_ptr(const T ptr) {
    return is_encoded_cc_ptr((const uint64_t)ptr);
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
 * @brief Check if ptr is encoded, and if so, decode it
 *
 * @tparam T type convertible to a uint64_t
 * @param ptr
 * @return T
 */
template <typename T> static inline T cc_dec_if_encoded_ptr(T ptr) {
    return is_encoded_cc_ptr(ptr) ? cc_isa_decptr(ptr) : ptr;
}

#endif  // defined(__cplusplus)

#endif  // MALLOC_CC_GLOBALS_H_
