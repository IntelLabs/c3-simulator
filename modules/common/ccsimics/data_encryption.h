/*
 Copyright 2016 Intel Corporation
 SPDX-License-Identifier: MIT
*/
#ifndef MODULES_COMMON_CCSIMICS_DATA_ENCRYPTION_H_
#define MODULES_COMMON_CCSIMICS_DATA_ENCRYPTION_H_

#include <simics/model-iface/cpu-instrumentation.h>
#include "ccsimics/cc_encoding.h"
#include "ccsimics/simics_util.h"
#include "crypto/ascon_cipher.h"

static inline void get_countermode_mask(ptr_metadata_t *metadata,
                                        logical_address_t la_encoded,
                                        const data_key_t *data_key,
                                        size_t num_bytes,
                                        uint8_t *countermode_mask) {
    uint64_t *mask64 = reinterpret_cast<uint64_t *>(countermode_mask);
    uint64_t la_base = (la_encoded >> CIPHER_OFFSET_BITS) << CIPHER_OFFSET_BITS;
    while (la_base < la_encoded + static_cast<uint64_t>(num_bytes)) {
        // *mask64 = K_cipher_64_enc(la_base, data_key->schedule);
        *mask64 = ascon64b_stream(la_base, data_key->bytes_);
        mask64++;
        la_base += (0x1ULL << CIPHER_OFFSET_BITS);
    }
}

static inline cpu_bytes_t encrypt_decrypt_bytes(ptr_metadata_t *metadata,
                                                logical_address_t la_encoded,
                                                const data_key_t *data_key,
                                                cpu_bytes_t bytes,
                                                uint8 *bytes_buffer) {
    uint8_t countermode_mask_unaligned[64 + 8];
    get_countermode_mask(metadata, la_encoded, data_key, bytes.size,
                         countermode_mask_unaligned);
    cpu_bytes_t bytes_mod;
    bytes_mod.size = bytes.size;
    int offset = static_cast<int>(la_encoded & 0x7);
    for (int i = bytes.size - 1; i >= 0; i--) {
        bytes_buffer[i] =
                bytes.data[i] ^ countermode_mask_unaligned[i + offset];
    }
    bytes_mod.data = bytes_buffer;
    return bytes_mod;
}

#endif  // MODULES_COMMON_CCSIMICS_DATA_ENCRYPTION_H_
