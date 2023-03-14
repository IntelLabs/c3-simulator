/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 */

/**
 * File: cclib.h
 *
 * Description: API functions used with programs modeling CC usage
 *
 * Original Authors: Andrew Weiler, Sergej Deutsch
 */

#ifndef MALLOC_CCLIB_H_
#define MALLOC_CCLIB_H_

#include <stddef.h>
#include <stdint.h>

void cc_init(void);

void *cc_malloc(size_t size);
void *cc_calloc(size_t num, size_t size);
void *cc_realloc(void *ptr, size_t new_size);
void cc_free(void *ptr);
void *cc_memalign(size_t alignment, size_t size);
void *cc_valloc(size_t size);
void *cc_pvalloc(size_t size);
int cc_posix_memalign(void **memptr, size_t alignment, size_t size);

#endif  // MALLOC_CCLIB_H_
