// Copyright 2016-2024 Intel Corporation
// SPDX-License-Identifier: MIT

// Reference implementations of functions box, malloc, calloc, realloc, etc.
// These implementations are environment agnostic; it should not depend if
// glibc or wrappers are used.
// External file must define
//   - REAL_MALLOC
//   - REAL_FREE
//   - MALLOC_USABLE_SIZE
//   - CC_ARCH_NOT_SUPPORT_MEMORY_ACCESS (if needed)
//
// The following runtime switches are also required:
//   - cc_debug_print
//   - cc_quarantine_enabled
//
// The following headers must be included:
//   - encoding.h
//   - The headers required for REAL_* functions
// This file should not be compiled directly! Import it with #include

#include <stdio.h>
#include <stdlib.h>
#include "cc_globals.h"  // NOLINT
// Helper function to unset ICV values before free()
static void *cc_pre_real_free_clear_icv(void *ptr) {
#ifdef CC_DETECT_1B_OVF
    uintptr_t ptr_int = (uintptr_t)ptr;
    ptr_int &= ICV_ADDR_MASK;
    ptr = (void *)ptr_int;
#endif
#ifdef CC_ICV_ENABLE
    // Clear ICV for old allocation
    if (!is_encoded_cc_ptr((uint64_t)ptr)) {
        size_t old_malloc_size = MALLOC_USABLE_SIZE(ptr);
        cc_set_icv(ptr, old_malloc_size);
    }
#endif  // CC_ICV_ENABLE
    return ptr;
}

#define ZERO_ON_ALLOC 0

/**
 * @brief If set to 1, the memory allocated by malloc will be zeroed out.
 *
 * NOTE that functions such as realloc require separate handling as they may
 * reuse previously allocated memory and thus cannot be zeroed.
 */
static const int kZeroOnAlloc = ZERO_ON_ALLOC;

static inline void *call_real_malloc(size_t size) { return REAL_MALLOC(size); }

static void *quarantine_buffers[QUARANTINE_WIDTH][QUARANTINE_DEPTH] = {0};
static int quarantine_current_buffer = 0;
static int quarantine_current_index = 0;

#define Q_NEXT(idx, max) (idx + 1) % max

// quarantine_buffer_free
// Description: Frees an entire buffer of quarantined pointers
void quarantine_buffer_free(int buf_index) {
    kprintf(stderr, "QUARANTINE: Freeing buffer %d\n", buf_index);
    for (int i = 0; i < QUARANTINE_DEPTH; i++) {
        cc_pre_real_free_clear_icv(quarantine_buffers[buf_index][i]);
        REAL_FREE(quarantine_buffers[buf_index][i]);
    }
}

// quarantine_insert
// Description: Ingests a pointer into quarantine_buffers. Triggers a buffer
// free
//   if the buffers are full.
void quarantine_insert(void *ptr) {
    if (!cc_quarantine_enabled) {
        cc_pre_real_free_clear_icv(ptr);
        REAL_FREE(ptr);
        return;
    }
    quarantine_buffers[quarantine_current_buffer][quarantine_current_index] =
            ptr;
    quarantine_current_index =
            Q_NEXT(quarantine_current_index, QUARANTINE_DEPTH);
    if (quarantine_current_index == 0) {
        quarantine_current_buffer =
                Q_NEXT(quarantine_current_buffer,
                       QUARANTINE_WIDTH);  // circular buffer + 1
        quarantine_buffer_free(quarantine_current_buffer);
    }
}

static uint8_t curr_version = 0;

void assign_version(uint64_t ptr, size_t size, ptr_metadata_t *ptr_metadata) {
    ptr_metadata->version_ = curr_version;

    curr_version = (curr_version + 1) % (1 << VERSION_SIZE);
    return;
}

void *cc_malloc_encoded(size_t size) {
    void *ptr = call_real_malloc(size);

    if (!ptr)
        return NULL;
    // kprintf(stderr, "GLIBC WRAPPER: original malloc(%d) pointer =%p\n", (int)
    // size, ptr);
    size_t malloc_size = MALLOC_USABLE_SIZE(ptr);
    ptr_metadata_t ptr_metadata = {0};
    void *p_encoded;
    if (try_box((uint64_t)ptr, malloc_size, &ptr_metadata)) {
        assign_version((uint64_t)ptr, malloc_size, &ptr_metadata);
#ifdef USE_CC_ISA
        p_encoded = (void *)cc_isa_encptr((uint64_t)ptr, &ptr_metadata);
#else
        p_encoded = (void *)encode_pointer((uint64_t)ptr, &ptr_metadata,
                                           &pointer_key);
#endif  // !USE_CC_ISA
    } else {
        p_encoded = ptr;
    }
#ifdef CC_ICV_ENABLE
    p_encoded =
            cc_setup_icv_for_alloc(p_encoded, size, malloc_size, kZeroOnAlloc);
#endif  // CC_ICV_ENABLE
    kprintf(stderr, "GLIBC WRAPPER: encoded malloc(%ld) = %p into %p\n", size,
            ptr, p_encoded);
    return p_encoded;
}

void *cc_calloc_encoded(size_t num, size_t size) {
    size_t total_size = num * size;

    void *ptr = call_real_malloc(total_size);

    if (!ptr)
        return NULL;
    size_t malloc_size = MALLOC_USABLE_SIZE(ptr);
    ptr_metadata_t ptr_metadata = {0};
    void *p_encoded;
    if (try_box((uint64_t)ptr, malloc_size, &ptr_metadata)) {
        assign_version((uint64_t)ptr, malloc_size, &ptr_metadata);
#ifdef USE_CC_ISA
        p_encoded = (void *)cc_isa_encptr((uint64_t)ptr, &ptr_metadata);
#else
        p_encoded = (void *)encode_pointer((uint64_t)ptr, &ptr_metadata,
                                           &pointer_key);
#endif  // !USE_CC_ISA
    } else {
        p_encoded = ptr;
    }

#ifdef CC_ICV_ENABLE
    p_encoded = cc_setup_icv_for_alloc(p_encoded, total_size, malloc_size, 1);
#elif !defined(CC_ARCH_NOT_SUPPORT_MEMORY_ACCESS)
    // Encrypted Memset
    uint8_t *dst = (uint8_t *)p_encoded;
    for (int i = 0; i < num * size; i++) {
        dst[i] = 0x0;
    }
#endif  // CC_ICV_ENABLE

    kprintf(stderr, "GLIBC WRAPPER: encoded calloc(%d, %d) = %p into %p\n",
            (int)num, (int)size, ptr, p_encoded);
    return p_encoded;
}

void *cc_realloc_encoded(void *p_old_encoded, size_t size) {
    void *p_old_decoded;
    size_t size_old;
    if (is_encoded_cc_ptr((uint64_t)p_old_encoded)) {
#ifdef USE_CC_ISA
        p_old_decoded = (void *)cc_isa_decptr((uint64_t)p_old_encoded);
#else
        p_old_decoded =
                (void *)decode_pointer((uint64_t)p_old_encoded, &pointer_key);
#endif  // USE_CC_ISA

        void *p_old_decoded_adj = p_old_decoded;
#if defined(CC_ICV_ENABLE) && defined(CC_DETECT_1B_OVF)
        uintptr_t p_old_decoded_adj_int = (uintptr_t)p_old_decoded_adj;
        p_old_decoded_adj_int &= ICV_ADDR_MASK;
        p_old_decoded_adj = (void *)p_old_decoded_adj_int;
#endif
        size_t old_malloc_size = MALLOC_USABLE_SIZE(p_old_decoded_adj);

        // Min of size implied by encoded pointer and implied by glibc metadata
        size_old = min_size(get_size_in_bytes((uint64_t)p_old_encoded),
                            old_malloc_size);

    } else {
        p_old_decoded = p_old_encoded;
        size_old = MALLOC_USABLE_SIZE(p_old_encoded);
    }

    size_t size_to_alloc = size;

    void *p_new_decoded = call_real_malloc(size_to_alloc);

    if (!p_new_decoded)
        return NULL;  // POSIX Compliance: Realloc returns NULL if the
                      // operation is unsuccessful, and the old memory is not
                      // freed/altered

    size_t new_malloc_size = MALLOC_USABLE_SIZE(p_new_decoded);
    ptr_metadata_t ptr_metadata = {0};
    void *p_new_encoded;
    if (try_box((uint64_t)p_new_decoded, new_malloc_size, &ptr_metadata)) {
        assign_version((uint64_t)p_new_decoded, new_malloc_size, &ptr_metadata);
#ifdef USE_CC_ISA
        p_new_encoded =
                (void *)cc_isa_encptr((uint64_t)p_new_decoded, &ptr_metadata);
#else
        p_new_encoded = (void *)encode_pointer((uint64_t)p_new_decoded,
                                               &ptr_metadata, &pointer_key);
#endif  // USE_CC_ISA
    } else {
        p_new_encoded = p_new_decoded;
    }

#ifdef CC_ICV_ENABLE
    // Setup ICV for allocation
    p_new_encoded = cc_setup_icv_for_alloc(p_new_encoded, size, new_malloc_size,
                                           kZeroOnAlloc);
#endif
#ifndef CC_ARCH_NOT_SUPPORT_MEMORY_ACCESS
    // Encrypted memcpy
    uint8_t *src = (uint8_t *)p_old_encoded;
    uint8_t *dst = (uint8_t *)p_new_encoded;
    size_t size_to_copy = min_size(size, size_old);

    if (size_to_copy != 0) {
        assert(src != NULL);
        assert(dst != NULL);
    }
    for (int i = 0; i < size_to_copy; i++) {
        dst[i] = src[i];
    }
#endif

    p_old_decoded = cc_pre_real_free_clear_icv(p_old_decoded);
    REAL_FREE(p_old_decoded);
    kprintf(stderr, "***** cc_realloc *****\n");
    kprintf(stderr, "p_old_encoded=0x%016lx\n", (uint64_t)p_old_encoded);
    kprintf(stderr, "p_old_decoded=0x%016lx\n", (uint64_t)p_old_decoded);
    kprintf(stderr, "p_new_decoded=0x%016lx\n", (uint64_t)p_new_decoded);
    kprintf(stderr, "p_new_encoded=0x%016lx\n", (uint64_t)p_new_encoded);
    kprintf(stderr, "size_old    =%ld\n", size_old);
    kprintf(stderr, "new_size    =%ld\n", size);
    return p_new_encoded;
}

void cc_free_encoded(void *p_in) {
    void *ptr;
    if (is_encoded_cc_ptr((uint64_t)p_in)) {
#ifdef USE_CC_ISA
        uint64_t ptr64 = cc_isa_decptr((uint64_t)p_in);
#else
        uint64_t ptr64 = decode_pointer((uint64_t)p_in, &pointer_key);
#endif  // USE_CC_ISA
        ptr = (void *)ptr64;
    } else {
        ptr = p_in;
    }
    kprintf(stderr, "GLIBC WRAPPER: free(%p) for %p\n", p_in, ptr);
    // Need to reset ICV regardless of quarantine for now, the reason being
    // that the quarantine buffer is not cleared on exit, potentially leaving
    // ICVs around even on otherwise clean exit.
    ptr = cc_pre_real_free_clear_icv(ptr);
    quarantine_insert(ptr);
}
