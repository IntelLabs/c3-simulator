// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef C3LIB_C3_RISC_V_RISC_V_CA_H_
#define C3LIB_C3_RISC_V_RISC_V_CA_H_

static inline uint64_t cc_isa_encptr(uint64_t ptr, const ptr_metadata_t *) {
    asm("BAD INSTRUCTION, NOT IMPLEMENTED");
    return ptr;
}

static inline uint64_t cc_isa_decptr(uint64_t ptr) {
    asm("BAD INSTRUCTION, NOT IMPLEMENTED");
    return ptr;
}

#endif  // C3LIB_C3_RISC_V_RISC_V_CA_H_
