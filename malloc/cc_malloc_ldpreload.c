/*
 Copyright 2020 Intel Corporation
 SPDX-License-Identifier: MIT
*/
#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdint.h>  // for uint**
#include <stdio.h>
#include <string.h>  // for memset

#define ADDR_MASK 0x0001000000000000
// #define ADDR_MASK 0x0000000000000000

// #define DATA_MASK 0xFF
#define DATA_MASK 0x00

static void *(*real_malloc)(size_t) = NULL;
static void *(*real_calloc)(size_t, size_t) = NULL;
static void *(*real_realloc)(void *, size_t) = NULL;
static void (*real_free)(void *) = NULL;

static inline int is_tagged_pointer(void *p) {
    return ((uint64_t)p & 0xFFFF000000000000) == ADDR_MASK ? 1 : 0;
}

static inline void *transform_pointer(void *p) {
    return (void *)((uint64_t)p ^ ADDR_MASK);
}

static void map_real_malloc() {
    real_malloc = dlsym(RTLD_NEXT, "malloc");
    if (NULL == real_malloc) {
        fprintf(stderr, "GLIBC WRAPPER: Error in `dlsym`: %s\n", dlerror());
    }
}

static void map_real_calloc() {
    real_calloc = dlsym(RTLD_NEXT, "calloc");
    if (NULL == real_calloc) {
        fprintf(stderr, "GLIBC WRAPPER: Error in `dlsym`: %s\n", dlerror());
    }
}

static void map_real_realloc() {
    real_realloc = dlsym(RTLD_NEXT, "realloc");
    if (NULL == real_realloc) {
        fprintf(stderr, "GLIBC WRAPPER: Error in `dlsym`: %s\n", dlerror());
    }
}

static void map_real_free() {
    real_free = dlsym(RTLD_NEXT, "free");
    if (NULL == real_free) {
        fprintf(stderr, "GLIBC WRAPPER: Error in `dlsym`: %s\n", dlerror());
    }
}

void *malloc(size_t size) {
    if (real_malloc == NULL)
        map_real_malloc();
    fprintf(stderr, "GLIBC WRAPPER: malloc(%d) = ", (int)size);
    void *p = real_malloc(size);
    void *p_transformed = NULL;
    if (size < 11)
        p_transformed = transform_pointer(p);
    else
        p_transformed = p;
    fprintf(stderr, "%p\n", p_transformed);
    return p_transformed;
}

void *calloc(size_t num, size_t size) {
    if (real_calloc == NULL)
        map_real_calloc();
    fprintf(stderr, "GLIBC WRAPPER: calloc(%d, %d) = ", (int)num, (int)size);
    void *p = real_calloc(num, size);
    void *p_transformed = NULL;
    if (size < 11)
        p_transformed = transform_pointer(p);
    else
        p_transformed = p;
    memset(p_transformed, 0x00, num * size);
    fprintf(stderr, "%p\n", p_transformed);
    return p_transformed;
}

void *realloc(void *p_old, size_t size) {
    if (is_tagged_pointer(p_old)) {
        if (real_realloc == NULL)
            map_real_realloc();
        void *p_new = NULL;
        fprintf(stderr, "GLIBC WRAPPER: realloc(%d) = ", (int)size);
        p_old = transform_pointer(p_old);
        p_new = real_realloc(p_old, size);
        fprintf(stderr, "%p\n", p_new);
        return transform_pointer(p_new);
    } else {
        if (real_realloc == NULL)
            map_real_realloc();
        void *p_new = real_realloc(p_old, size);
        return p_new;
    }
}

void free(void *p) {
    if (p == NULL)
        return;
    if (real_free == NULL)
        map_real_free();
    fprintf(stderr, "GLIBC WRAPPER: free(%p)\n", p);
    void *p_untagged = is_tagged_pointer(p) ? transform_pointer(p) : p;
    real_free(p_untagged);
}
