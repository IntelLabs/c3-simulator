// Copyright 2021-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef MALLOC_LIM_PTR_ENCODING_H_
#define MALLOC_LIM_PTR_ENCODING_H_

#include <assert.h>
#include <malloc.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern int lim_no_encode;
extern int trace_only;

#define HEAP_MIN_CHUNK_SZ 24

// #define LIM_METADATA_OFFSET_FROM_MIDDLE 0

#define LIM_METADATA_SIZE_32_64B 1
#define LIM_METADATA_OFFSET_FROM_MIDDLE_32_64B 0
#define LIM_ENCODING_LIMIT_1 64

#define LIM_METADATA_SIZE_128_256B 2
#define LIM_METADATA_OFFSET_FROM_MIDDLE_128_256B 1
#define LIM_ENCODING_LIMIT_2 256

#define LIM_METADATA_SIZE_512B 16
#define LIM_METADATA_OFFSET_FROM_MIDDLE_512B 8

// LIM bounds are 8-byte-granular, so stored bounds are shifted by 3 bits:
#define LIM_BOUND_ENC_OFFSET 3
#define LIM_TAG_BITS 4
#define LIM_SIZE_ENC_BITS 6

// LIM_MAX_LA_WIDTH < an object may not cross the user/supervisor boundary, so
// the max LA width is one less than the machine's supported LA width
#define LIM_MAX_LA_WIDTH 56
// LIM_MIN_PWR_SZ < minimum supported power size
#define LIM_MIN_PWR_SZ 5
/// Minimum encoding for size value that can be embedded in pointer.
/// This could have been defined to differ from LIM_MIN_PWR_SZ, but the encoding
/// space in the power size field is more than large enough for the total number
/// of power sizes that need to be encoded, so we may as well directly encode
/// the the power size into the pointer. Note that the minimumum is non-zero,
/// which preserves the all-0 power size field value to indicate an unencoded
/// pointer when the pointer overall is canonical.
#define LIM_MIN_ENCODED_SZ LIM_MIN_PWR_SZ
#define LIM_MAX_ENCODED_SZ LIM_MAX_LA_WIDTH

#define LIM_MIN_SLOT_SZ (1ull << LIM_MIN_PWR_SZ)

#define PAGE_OFFSET 12
#ifdef PAGE_SIZE
#define SIM_PAGE_SIZE PAGE_SIZE
#else
#define SIM_PAGE_SIZE (1 << PAGE_OFFSET)
#endif
#define SIM_CACHELINE_OFFSET 6
#define SIM_CACHELINE_SIZE (1 << SIM_CACHELINE_OFFSET)
#define LIM_FMASK(x) (0xFFFFFFFFFFFFFFFF >> (64 - x))

typedef uint8_t lim_tag_t;

typedef struct {
    uint64_t base_addr : 64 - LIM_TAG_BITS - LIM_SIZE_ENC_BITS;
    uint64_t tag : LIM_TAG_BITS;
    uint64_t enc_size : LIM_SIZE_ENC_BITS;
} encoded_pointer_t;

typedef struct {
    uint8_t tag : LIM_TAG_BITS;
    uint8_t lower_bound : 2;
    uint8_t upper_bound : 2;
} lim_meta_1B_t;

typedef struct {
    uint16_t lower_bound : 4;
    uint16_t tag_left : LIM_TAG_BITS;
    uint16_t tag_right : LIM_TAG_BITS;
    uint16_t upper_bound : 4;
} lim_meta_2B_t;

typedef struct {
    uint64_t lower_bound : 30;
    uint64_t tag : LIM_TAG_BITS;
    uint64_t upper_bound : 30;
} lim_meta_8B_t;

typedef struct {
    uint64_t lower_bound : 60;
    uint64_t tag_left : LIM_TAG_BITS;
    uint64_t tag_right : LIM_TAG_BITS;
    uint64_t upper_bound : 60;
} lim_meta_16B_t;

static inline int min_int(int a, int b) { return (a < b) ? a : b; }
static inline uint64_t min_uint64(uint64_t a, uint64_t b) {
    return (a < b) ? a : b;
}
static inline uint64_t max_uint64(uint64_t a, uint64_t b) {
    return (a > b) ? a : b;
}

static inline void check_encoded_size(uint8_t encoded_size) {
    if (encoded_size < LIM_MIN_ENCODED_SZ ||
        LIM_MAX_ENCODED_SZ < encoded_size) {
        fprintf(stderr, "LIM: invalid encoded size: %u\n", encoded_size);
        abort();
    }
}

static inline int is_encoded_lim_ptr(uint64_t la) {
    encoded_pointer_t *ep = (encoded_pointer_t *)&la;
    return (ep->enc_size != 0 && ep->enc_size != LIM_FMASK(LIM_SIZE_ENC_BITS))
                   ? 1
                   : 0;
}
static inline uint64_t lim_decode_pointer(uint64_t encoded_pointer) {
    encoded_pointer_t *ep = (encoded_pointer_t *)&encoded_pointer;
    return (uint64_t)ep->base_addr;
}
static inline uint64_t lim_encode_pointer(uint64_t addr, uint8_t encoded_size,
                                          lim_tag_t tag) {
    check_encoded_size(encoded_size);
    encoded_pointer_t *ep = (encoded_pointer_t *)&addr;
    ep->enc_size = encoded_size;
    ep->tag = tag;
    return addr;
}
static inline lim_tag_t get_encoded_tag(uint64_t encoded_pointer) {
    encoded_pointer_t *ep = (encoded_pointer_t *)&encoded_pointer;
    return (uint8_t)ep->tag;
}
static inline uint8_t get_encoded_size(uint64_t encoded_pointer) {
    encoded_pointer_t *ep = (encoded_pointer_t *)&encoded_pointer;
    return (uint8_t)ep->enc_size;
}
static inline unsigned get_slot_size_bit_shift(uint8_t encoded_size) {
    check_encoded_size(encoded_size);
    return encoded_size;
}
static inline size_t get_slot_size_in_bytes(uint8_t encoded_size) {
    return 1ULL << get_slot_size_bit_shift(encoded_size);
}
static inline uint64_t get_middle_address(uint64_t addr, uint8_t encoded_size) {
    unsigned int shift_bits = get_slot_size_bit_shift(encoded_size);
    return ((addr >> shift_bits) << shift_bits) | (1ULL << (shift_bits - 1));
}
static inline uint64_t get_metadata_offset_from_middle(uint8_t encoded_size) {
    size_t slot_size = get_slot_size_in_bytes(encoded_size);
    if (slot_size <= LIM_ENCODING_LIMIT_1) {
        return LIM_METADATA_OFFSET_FROM_MIDDLE_32_64B;
    } else if (slot_size <= LIM_ENCODING_LIMIT_2) {
        return LIM_METADATA_OFFSET_FROM_MIDDLE_128_256B;
    } else {
        return LIM_METADATA_OFFSET_FROM_MIDDLE_512B;
    }
}
static uint64_t get_metadata_address(uint64_t addr, uint8_t encoded_size) {
    uint64_t middle_addr = get_middle_address(addr, encoded_size);
    return (middle_addr - get_metadata_offset_from_middle(encoded_size));
}
__attribute__((unused)) static uint64_t
get_metadata_address_from_enc_ptr(uint64_t enc_ptr) {
    uint64_t p_dec = lim_decode_pointer(enc_ptr);
    uint8_t enc_sz = get_encoded_size(enc_ptr);
    return get_metadata_address(p_dec, enc_sz);
}
static inline uint64_t get_slot_start(uint64_t addr, uint8_t encoded_size) {
    unsigned int shift_bits = get_slot_size_bit_shift(encoded_size);
    return (addr >> shift_bits) << shift_bits;
}
static inline uint64_t get_slot_end(uint64_t addr, uint8_t encoded_size) {
    unsigned int shift_bits = get_slot_size_bit_shift(encoded_size);
    uint64_t next_slot_start = ((addr >> shift_bits) + 1) << shift_bits;
    return next_slot_start - 1;
}
static inline uint64_t get_page_addr(uint64_t addr) {
    return (addr >> PAGE_OFFSET);
}
static inline uint64_t get_next_page_start_addr(uint64_t addr) {
    return (get_page_addr(addr + SIM_PAGE_SIZE)) << PAGE_OFFSET;
}
static inline uint64_t get_next_cacheline_start_addr(uint64_t addr) {
    return (get_page_addr(addr + SIM_CACHELINE_SIZE)) << SIM_CACHELINE_OFFSET;
}
static uint8_t calculate_encoded_size(uint64_t ptr, size_t size) {
    if (size < HEAP_MIN_CHUNK_SZ) {
        size = HEAP_MIN_CHUNK_SZ;
    }

    size_t max_off = size - 1;
    uint64_t ptr_end = ptr + max_off;
    uint64_t diff = ptr ^ ptr_end;
    int leading_zeros_in_diff = __builtin_clzl(diff);
    uint8_t encoded_size = (uint8_t)(64 - leading_zeros_in_diff);
    if (encoded_size <= LIM_MIN_ENCODED_SZ) {
        encoded_size = LIM_MIN_ENCODED_SZ;
    }
    check_encoded_size(encoded_size);
    return encoded_size;
}

static inline size_t get_min_metadata_size(size_t data_size) {
    if (data_size + LIM_METADATA_SIZE_32_64B <= LIM_ENCODING_LIMIT_1) {
        return LIM_METADATA_SIZE_32_64B;
    } else if (data_size + LIM_METADATA_SIZE_128_256B <= LIM_ENCODING_LIMIT_2) {
        return LIM_METADATA_SIZE_128_256B;
    } else {
        return LIM_METADATA_SIZE_512B;
    }
}

__attribute__((unused)) static size_t get_metadata_size(uint8_t encoded_size) {
    size_t slot_size = get_slot_size_in_bytes(encoded_size);
    if (slot_size <= LIM_ENCODING_LIMIT_1) {
        return LIM_METADATA_SIZE_32_64B;
    } else if (slot_size <= LIM_ENCODING_LIMIT_2) {
        return LIM_METADATA_SIZE_128_256B;
    } else {
        return LIM_METADATA_SIZE_512B;
    }
}

static inline size_t get_next_larger_metadata_size(size_t meta_size) {
    switch (meta_size) {
    case 0:
        return LIM_METADATA_SIZE_32_64B;
    case LIM_METADATA_SIZE_32_64B:
        return LIM_METADATA_SIZE_128_256B;
    case LIM_METADATA_SIZE_128_256B:
        return LIM_METADATA_SIZE_512B;
    default:
        fprintf(stderr, "Invalid metadata size: %lu\n", meta_size);
        exit(1);
    }
}

/// Expects an inclusive byte-granular bound as input expressed as a distance
/// beyond the first byte on the respective side of the midpoint.
///
/// Must be able to encode a range from 8 bytes to 2^|bnd| bytes,
/// where |bnd| is the bitwidth of each bound field.  Otherwise, we would
/// only be able to encode the range [0,(2^|bnd|)-8].
/// Input bounds are rounded up to the next
/// 8B boundary.
static inline uint64_t encode_bound(uint64_t bound) {
    return bound >> LIM_BOUND_ENC_OFFSET;
}
static inline uint64_t decode_bound(uint64_t bound) {
    return ((bound + 1) << LIM_BOUND_ENC_OFFSET);
}

/// Return the number of data bytes (i.e. excluding metadata storage) within the
/// encoded bounds, which are aligned to 8-byte boundaries.  Thus, the return
/// value may exceed the input data_size.
__attribute__((unused)) static size_t
lim_compute_bounded_data_size(void *ptr, size_t data_size, size_t meta_size,
                              uint64_t *out_enc_lb, uint64_t *out_enc_ub) {
    uint8_t encoded_size =
            calculate_encoded_size((uint64_t)ptr, data_size + meta_size);
    size_t slot_size = get_slot_size_in_bytes(encoded_size);
    uint64_t ptr_midpoint = get_middle_address((uint64_t)ptr, encoded_size);
    void *ptr_metadata =
            (void *)get_metadata_address((uint64_t)ptr, encoded_size);
    (void)ptr_metadata;
    assert((uint64_t)ptr_metadata > (uint64_t)ptr);
    assert((uint64_t)ptr_metadata <= (uint64_t)ptr_midpoint);
    // inclusive lower bound that will be rounded up to 8B boundary:
    // If the allocation does not reach the slot midpoint, this lower bound
    // computation will effectively "stretch" it to that point.  In other words,
    // overflows prior to the midpoint will not be detected in that case.
    // However, the next assertion will reject any allocations that could have
    // been fit to a tighter slot, so they will not be allowed to be stretched
    // in this way.  It is only allowed for allocations in the smallest slot
    // size.
    uint64_t lower_bound_in_bytes = (ptr_midpoint - 1) - (uint64_t)ptr;
    // inclusive upper bound that will be rounded up to 8B boundary:
    // The bounds encoding is such that the upper bound needs to extend at least
    // 8B from the midpoint.  Thus, even if an allocation does not extend beyond
    // its metadata, the upper bound will still include the 8B just beyond the
    // midpoint.  This may effectively stretch allocations' upper bounds.
    // However, note that allocations with 16B metadata will never be stretched
    // in this way, because the 8B of metadata itself above the midpoint
    // satisfies the minimum upper bound requirement.
    uint64_t upper_bound_in_bytes = 0;
    uint64_t upper_la = (uint64_t)ptr + data_size + meta_size - 1;
    if (ptr_midpoint <= upper_la) {
        upper_bound_in_bytes = upper_la - ptr_midpoint;
    } else {
        // For larger data sizes, a tighter slot should have been used.
        assert(data_size <= LIM_MIN_SLOT_SZ / 2);
        // There is always at least one byte of metadata beyond the midpoint,
        // but encode_bound rounds an inclusive byte bound of 0 up to 8, so
        // leaving upper_bound_in_bytes set to zero is acceptable.
    }
    // fprintf(stderr, ">>>>>> \n");
    // fprintf(stderr, "%s: data size            = %ld\n", __func__, data_size);
    // fprintf(stderr, "%s: meta size            = %ld\n", __func__, meta_size);
    // fprintf(stderr, "%s: slot_size            = %ld\n", __func__, slot_size);
    // fprintf(stderr, "%s: encoded_size         = %x\n", __func__,
    // encoded_size); fprintf(stderr, "%s: ptr                  = %p\n",
    // __func__, ptr); fprintf(stderr, "%s: ptr_metadata         = %p\n",
    // __func__, ptr_metadata); fprintf(stderr, "%s: lower_bound_in_bytes =
    // %lx\n", __func__, lower_bound_in_bytes); fprintf(stderr, "%s:
    // upper_bound_in_bytes = %lx\n", __func__, upper_bound_in_bytes);

    uint64_t enc_lb = encode_bound(lower_bound_in_bytes);
    uint64_t enc_ub = encode_bound(upper_bound_in_bytes);

    if (out_enc_lb)
        *out_enc_lb = enc_lb;
    if (out_enc_ub)
        *out_enc_ub = enc_ub;

    uint64_t dec_lb = decode_bound(enc_lb);
    uint64_t dec_ub = decode_bound(enc_ub);

    assert(dec_lb <= slot_size / 2);
    assert(dec_ub <= slot_size / 2);

    if (slot_size <= LIM_ENCODING_LIMIT_1) {
        assert(meta_size == LIM_METADATA_SIZE_32_64B);
    } else if (slot_size <= LIM_ENCODING_LIMIT_2) {
        assert(meta_size == LIM_METADATA_SIZE_128_256B);
    } else {
        assert(meta_size == LIM_METADATA_SIZE_512B);
    }

    return dec_lb + dec_ub - meta_size;
}

/// Return the number of data bytes (i.e. excluding metadata storage) within the
/// encoded bounds, which are aligned to 8-byte boundaries.  Thus, the return
/// value may exceed the input data_size.
__attribute__((unused)) static size_t
set_metadata(void *ptr, size_t data_size, size_t meta_size, lim_tag_t tag) {
    uint8_t encoded_size =
            calculate_encoded_size((uint64_t)ptr, data_size + meta_size);
    void *ptr_metadata =
            (void *)get_metadata_address((uint64_t)ptr, encoded_size);

    uint64_t enc_lb, enc_ub;
    uint64_t bounded_data_sz = lim_compute_bounded_data_size(
            ptr, data_size, meta_size, &enc_lb, &enc_ub);
    if (meta_size == LIM_METADATA_SIZE_32_64B) {
        lim_meta_1B_t meta;
        assert(meta_size == sizeof(meta));
        meta.lower_bound = enc_lb;
        meta.upper_bound = enc_ub;
        meta.tag = tag;
        if (lim_no_encode) {
            // When simply modeling allocator overheads, do not actually change
            // the metadata, because this might be invoked from realloc and
            // corrupt data:
            memmove(ptr_metadata, ptr_metadata, sizeof(meta));
        } else {
            memcpy(ptr_metadata, &meta, sizeof(meta));
        }
    } else if (meta_size == LIM_METADATA_SIZE_128_256B) {
        lim_meta_2B_t meta;
        assert(meta_size == sizeof(meta));
        meta.lower_bound = enc_lb;
        meta.upper_bound = enc_ub;
        meta.tag_left = tag;
        meta.tag_right = tag;
        if (lim_no_encode) {
            memmove(ptr_metadata, ptr_metadata, sizeof(meta));
        } else {
            memcpy(ptr_metadata, &meta, sizeof(meta));
        }
    } else {
        assert(meta_size == LIM_METADATA_SIZE_512B);
        lim_meta_16B_t meta;
        assert(meta_size == sizeof(meta));
        meta.lower_bound = enc_lb;
        meta.upper_bound = enc_ub;
        meta.tag_left = tag;
        meta.tag_right = tag;
        if (lim_no_encode) {
            memmove(ptr_metadata, ptr_metadata, sizeof(meta));
        } else {
            memcpy(ptr_metadata, &meta, sizeof(meta));
        }
    }

    return bounded_data_sz;
}

typedef struct {
    uint8_t tag_left, tag_right;
    uint64_t lower_la;  // inclusive lower bound
    uint64_t upper_la;  // inclusive upper bound
} lim_decoded_metadata_t;

__attribute__((unused)) static lim_decoded_metadata_t
lim_decode_metadata(void *metadata, size_t meta_size, uintptr_t la_middle) {
    lim_decoded_metadata_t dec;

    if (trace_only) {
        dec.tag_left = dec.tag_right = 0;
        dec.lower_la = 0;
        dec.upper_la = ~0ull;

        return dec;
    }

    if (meta_size == sizeof(lim_meta_1B_t)) {
        lim_meta_1B_t *meta_p = (lim_meta_1B_t *)metadata;
        dec.tag_left = dec.tag_right = meta_p->tag;
        dec.lower_la = la_middle - decode_bound(meta_p->lower_bound);
        dec.upper_la = la_middle + decode_bound(meta_p->upper_bound) - 1;
    } else if (meta_size == sizeof(lim_meta_2B_t)) {
        lim_meta_2B_t *meta_p = (lim_meta_2B_t *)metadata;
        dec.tag_left = meta_p->tag_left;
        dec.tag_right = meta_p->tag_right;
        dec.lower_la = la_middle - decode_bound(meta_p->lower_bound);
        dec.upper_la = la_middle + decode_bound(meta_p->upper_bound) - 1;
    } else {
        assert(meta_size == sizeof(lim_meta_16B_t));
        lim_meta_16B_t *meta_p = (lim_meta_16B_t *)metadata;
        dec.tag_left = meta_p->tag_left;
        dec.tag_right = meta_p->tag_right;
        dec.lower_la = la_middle - decode_bound(meta_p->lower_bound);
        dec.upper_la = la_middle + decode_bound(meta_p->upper_bound) - 1;
    }

    return dec;
}

static size_t lim_compute_disp_size(uintptr_t meta_la, size_t meta_size,
                                    lim_decoded_metadata_t *dec_meta) {
    uintptr_t last_meta_la = meta_la + meta_size - 1;
    assert(last_meta_la <= dec_meta->upper_la);
    size_t space_after_meta = dec_meta->upper_la - last_meta_la;
    return (meta_size < space_after_meta) ? meta_size : space_after_meta;
}

__attribute__((unused)) static uintptr_t
lim_compute_disp_base(uintptr_t meta_la, size_t meta_size,
                      lim_decoded_metadata_t *dec_meta) {
    return (dec_meta->upper_la -
            lim_compute_disp_size(meta_la, meta_size, dec_meta)) +
           1;
}

#endif  // MALLOC_LIM_PTR_ENCODING_H_
