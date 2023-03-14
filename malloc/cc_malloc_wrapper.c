/*
 Copyright 2020 Intel Corporation
 SPDX-License-Identifier: MIT
*/

#include <malloc.h>  // for malloc_usable_size
#include <stdint.h>  // for uint**
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cc_globals.h"  // NOLINT
#include "utils.h"       // NOLINT
#if CC_POINTER_ENCODING >= POINTER_ENCRYPTION_V1
#include "encoding.h"  // NOLINT
#endif

#define COMPILE_FOR_SIMICS
#define REAL_MALLOC __real_malloc
#define REAL_FREE __real_free
#define MALLOC_USABLE_SIZE malloc_usable_size
#define cc_quarantine_enabled 0
void *__real_malloc(size_t size);
void *__real_realloc(void *p_tagged, size_t size);
// void *__wrap_calloc(size_t num, size_t size); // THIS DOES NOT WORK. USE
// MALLOC+MEMSET INSTEAD
void __real_free(void *ptr);

#ifndef DEBUG_PRINTF
#define DEBUG_PRINTF_VAL 0
#define cc_debug_print 0
#else
#define DEBUG_PRINTF_VAL 1
#define cc_debug_print 1
#endif
#define debug_printf(fmt, ...)                                                 \
    do {                                                                       \
        if (DEBUG_PRINTF_VAL)                                                  \
            fprintf(stderr, fmt, __VA_ARGS__);                                 \
    } while (0)

static pointer_key_t pointer_key;
static int initialized = 0;
#include "cc_malloc.c"  // NOLINT

void init() {
    initialized = 1;
    init_crypto_key_struct(&pointer_key);
    // printf("key initialized\n");
}

void *__wrap_malloc(size_t size) {
    if (!initialized)
        init();
    return cc_malloc_encoded(size);
    /*
    // OLD wrap implementation
    void *ptr = __real_malloc(size);
    debug_printf("GLIBC WRAPPER: original malloc(%d) pointer =%p\n", (int) size,
 ptr); #if CC_POINTER_ENCODING == POINTER_TAGGING void *p_encoded = set_tag(ptr,
 generate_tag()); #elif CC_POINTER_ENCODING == POINTER_ENCRYPTION_V1
        if(!initialized) init();
        ptr_metadata_t ptr_metadata;
 //       box( (uint64_t) ptr, size, &ptr_metadata);
        size_t malloc_size = get_allocation_size(ptr);
        if (exceeds_max_allocation(malloc_size)) {
            debug_printf("GLIBC WRAPPER: pointer not encoded (exceeds max CC
 pointer size) ptr = %p\n", ptr); return ptr;
        }
        box( (uint64_t) ptr, malloc_size, &ptr_metadata);
        void *p_encoded = (void*) encode_pointer( (uint64_t) ptr, &ptr_metadata,
 &pointer_key); #elif ERROR #endif debug_printf("GLIBC WRAPPER: encoded malloc
 pointer = %p\n", p_encoded); return p_encoded;
    */
}
void *__wrap_realloc(void *p_old_encoded, size_t size) {
    if (!initialized)
        init();
    return cc_realloc_encoded(p_old_encoded, size);
    /*
    if(!initialized) init();
    size_t size_old;
    void* p_old_decoded = NULL;
    if (is_encoded_pointer(p_old_encoded)){
        p_old_decoded = (void*) decode_pointer( (uint64_t) p_old_encoded,
    &pointer_key); size_old =
    min_size(get_size_in_bytes((uint64_t)p_old_encoded),
    get_allocation_size(p_old_decoded)); } else { p_old_decoded = p_old_encoded;
        size_old = get_allocation_size(p_old_encoded);
    }
    void *p_new_decoded = __real_malloc(size);
    size_t alloc_size_new = get_allocation_size(p_new_decoded);
    void* p_new_encoded = NULL;
    if (exceeds_max_allocation(alloc_size_new)){
        p_new_encoded = p_new_decoded;
    } else {
        ptr_metadata_t ptr_metadata;
        box( (uint64_t) p_new_decoded, alloc_size_new, &ptr_metadata);
        p_new_encoded = (void*) encode_pointer((uint64_t) p_new_decoded,
    &ptr_metadata, &pointer_key);
    }
    uint8_t* src = (uint8_t*) p_old_encoded;
    uint8_t* dst = (uint8_t*) p_new_encoded;
    for (int i = 0; i < min(size, size_old); i++) {
        dst[i] = src[i];
    }
    //memcpy(p_new_encoded, p_old_encoded, min(size, size_old));
    //printf("***** cc_realloc *****\n");
    //printf ("p_old_encoded=0x%016lx\n", (uint64_t) p_old_encoded);
    //printf ("p_old_decoded=0x%016lx\n", (uint64_t) p_old_decoded);
    //printf ("p_new_decoded=0x%016lx\n", (uint64_t) p_new_decoded);
    //printf ("p_new_encoded=0x%016lx\n", (uint64_t) p_new_encoded);
    //printf ("size_old    =%d\n", (int) size_old);
    //printf ("new_size    =%d\n", (int) size);
    __real_free(p_old_decoded);
    debug_printf("GLIBC WRAPPER: realloc(%d) = 0x%016lx\n", (int) size,
    (uint64_t) p_new_encoded); return p_new_encoded;
    */
}

void *__wrap_calloc(size_t num, size_t size) {
    if (!initialized)
        init();
    return cc_calloc_encoded(num, size);

    /*
    if(!initialized) init();
    size_t total_size = num * size;
    void *ptr = __real_malloc(total_size);
    size_t malloc_size = get_allocation_size(ptr);
    void *p_encoded = NULL;
    if (exceeds_max_allocation(malloc_size)) {
        debug_printf("GLIBC WRAPPER: pointer not encoded (exceeds max CC pointer
    size) ptr = %p\n", ptr); p_encoded = ptr; } else { ptr_metadata_t
    ptr_metadata; box( (uint64_t) ptr, malloc_size, &ptr_metadata); p_encoded =
    (void*) encode_pointer( (uint64_t) ptr, &ptr_metadata, &pointer_key);
    }
    uint8_t* dst = (uint8_t*) p_encoded;
    for (int i = 0; i <total_size; i++) {
        dst[i] = 0x0;
    }
    debug_printf("GLIBC WRAPPER: calloc(%d, %d) = %p\n", (int) num, (int) size,
    p_encoded); return p_encoded;
    */
}

void __wrap_free(void *p_in) {
    if (!initialized)
        init();
    cc_free_encoded(p_in);
    /*
    void *ptr = is_encoded_pointer(p_in) ? (void*) decode_pointer((uint64_t)
    p_in, &pointer_key) : p_in;
    __real_free(ptr);
    debug_printf("GLIBC WRAPPER: free(%p)\n", ptr);
    */
}

void *__wrap_memcpy(void *destination, const void *source, size_t num) {
    debug_printf("GLIBC WRAPPER: in memcpy(%p, %p, %lu)\n", destination, source,
                 num);

    uint8_t *udest = (uint8_t *)destination;
    const uint8_t *usrc = (uint8_t *)source;
    unsigned int count = num;

    while (count--)
        *udest++ = *usrc++;

    debug_printf("GLIBC WRAPPER: memcpy(%p, %p, %lu) completed.\n", destination,
                 source, num);
    return destination;
}
