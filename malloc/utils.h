/*
 Copyright 2021 Intel Corporation
 SPDX-License-Identifier: MIT
*/

/**
 * File: utils.h
 *
 * Description: Functions used in the cryptographic computing models.
 *
 */

#ifndef UTILS_H
#define UTILS_H
#include <stdlib.h>

static inline uint64_t round_up_pow2( uint64_t v ){
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v |= v >> 32;
	v++;
	return v;
}

#define CC_DEBUG_PRINT
#define CC_DEBUG

#ifdef CC_DEBUG_PRINT
    #include <stdio.h> 
    #define kprintf(...) if(cc_debug_print) fprintf(__VA_ARGS__)
#else
    #define kprintf(...)
#endif

#ifdef CC_DEBUG
    #define kassert assert
#else
    #define kassert(...)
#endif

static inline int min (int a, int b) {
    return (a < b) ? a : b;
}
static inline size_t min_size (size_t a, size_t b) {
    return (a < b) ? a : b;
}

static inline uint64_t rand64(void) {
    uint64_t high_bits = (uint64_t) rand() ;
    uint64_t low_bits = (uint64_t) rand() ;
    return (high_bits << 32) | low_bits;
}

#endif
