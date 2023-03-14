/*
 Copyright 2021 Intel Corporation
 SPDX-License-Identifier: MIT
*/

#ifndef MALLOC_NO_DEPENDENCY_ENCODING_H_
#define MALLOC_NO_DEPENDENCY_ENCODING_H_

#if defined(__cplusplus)
extern "C" {
#endif

#define ZTS_RSP_OFFSET_SIZE 22
#define ZTS_RSP_OFFSET_3b 3
#define ZTS_RSP_FULL_OFFSET_SIZE (ZTS_RSP_OFFSET_SIZE + ZTS_RSP_OFFSET_3b)

// FIXME: Potentially place this under an IFUNC hook that will alternatively
// invoke is_encoded_address when LIM is enabled:
static inline int is_encoded_pointer(const void *p) {
    return ((uint64_t)p & (0xFFFF000000000000ULL)) != 0;
}

static inline int is_encoded_ra(void *p) {
    uint64_t p_top = (uint64_t)p >> 48;
    return p_top != 0xFFFF && p_top != 0x0000;
}

static inline void *CC_CLEAN_RA(volatile void *p) {
    if (is_encoded_ra((void *)p)) {
        return (void *)((uint64_t)p & (0x0000FFFFFFFFFFFFULL));
    }
    return (void *)p;
}

static inline uint64_t zts_get_rsp_offset(const uint64_t p) {
    const uint64_t mask = ~(~0UL << (ZTS_RSP_FULL_OFFSET_SIZE));
    return p & mask;
}

#if defined(__cplusplus)
}
#endif
#endif  // MALLOC_NO_DEPENDENCY_ENCODING_H_
