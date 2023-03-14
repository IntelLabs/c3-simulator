/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 */

/**
 * File: cclib.c
 *
 * Description: Adapter for cc_malloc.c. Exposes malloc, calloc, free, etc.
 *
 * Original Authors: Andrew Weiler, Sergej Deutsch
 */

#include "cclib.h"  // NOLINT
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cc_encoding.h"  // NOLINT
#include "utils.h"        // NOLINT

// Global state variables
static pointer_key_t pointer_key;  // Which key to use for encoding.
static int initialized = 0;        // Will call init if this is still set to 0.

// Variables for enable/disable encoding
static uint64_t cc_debug_print = 0;
static int cc_quarantine_enabled = 1;

// Provide names for backend allocator functions for cc_malloc.c
#define REAL_MALLOC malloc
#define REAL_FREE free
#define MALLOC_USABLE_SIZE malloc_usable_size
#define CC_ARCH_NOT_SUPPORT_MEMORY_ACCESS  // Assume encoded addresses cannot be
                                           // accessed.
// Import actual encoded definitions for malloc, calloc, realloc.
#include "cc_malloc.c"  // NOLINT

void init(void) {
    init_crypto_key_struct(&pointer_key);
    cc_debug_print = (getenv("CC_DEBUG_PRINT") != NULL);
    cc_quarantine_enabled = !(getenv("CC_NO_QUARANTINE") != NULL);

    initialized = 1;
    // printf("key initialized\n");
}

void *cc_malloc(size_t size) {
    if (!initialized)
        init();
    return cc_malloc_encoded(size);
}

void *cc_calloc(size_t num, size_t size) {
    if (!initialized)
        init();
    return cc_calloc_encoded(num, size);
}

void *cc_realloc(void *tmem, size_t tsize) {
    if (!initialized)
        init();
    return cc_realloc_encoded(tmem, tsize);
}

// No wrapper needed.
void cc_free(void *p_in) {
    cc_free_encoded(p_in);
    return;
}

void *cc_memalign(size_t alignment, size_t size) {
    void *ret;
    kprintf(stderr, "@cc_memalign, alignment = %ld size = %ld\n", alignment,
            size);
    abort();  // Break on unimplemented function
    ret = memalign(alignment, size);
    return ret;
}

void *cc_valloc(size_t size) {
    void *ret;
    kprintf(stderr, "@cc_valloc!\n");
    abort();  // Break on unimplemented function
    ret = valloc(size);
    return ret;
}

void *cc_pvalloc(size_t size) {
    void *ret;
    kprintf(stderr, "@cc_pvalloc!\n");
    abort();  // Break on unimplemented function
    ret = pvalloc(size);
    return ret;
}

int cc_posix_memalign(void **memptr, size_t alignment, size_t size) {
    void *pret;
    kprintf(stderr, "@cc_posix_memalign, alignment = %ld size = %ld\n",
            alignment, size);
    abort();  // Break on unimplemented function
    pret = memalign(alignment, size);
    if (pret) {
        *memptr = pret;
        return 0;
    } else {
        return ENOMEM;
    }
}
