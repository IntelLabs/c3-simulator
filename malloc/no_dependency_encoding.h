/*
 Copyright 2021 Intel Corporation
 SPDX-License-Identifier: MIT
*/

#ifndef NODEP_CC_ENCODINGS_H
#define NODEP_CC_ENCODINGS_H

// FIXME: Potentially place this under an IFUNC hook that will alternatively
// invoke is_encoded_address when LIM is enabled:
static inline int is_encoded_pointer(const void* p){
  return ((uint64_t)p & (0xFFFF000000000000ULL)) != 0;
}

#endif
