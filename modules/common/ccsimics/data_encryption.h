/*
 Copyright 2016 Intel Corporation
 SPDX-License-Identifier: MIT
*/
#ifndef MODULES_COMMON_CCSIMICS_DATA_ENCRYPTION_H_
#define MODULES_COMMON_CCSIMICS_DATA_ENCRYPTION_H_

#include <simics/model-iface/cpu-instrumentation.h>
#include "ccsimics/simics_util.h"
#include "crypto/ascon_cipher.h"
#include "crypto/cc_encoding.h"

static inline cpu_bytes_t encrypt_decrypt_bytes(ptr_metadata_t *metadata,
                                                logical_address_t la_encoded,
                                                const data_key_t *data_key,
                                                cpu_bytes_t bytes,
                                                uint8 *bytes_buffer) {
    CCDataEncryption::encrypt_decrypt_bytes(la_encoded, data_key, bytes.data,
                                            bytes_buffer, bytes.size);

    cpu_bytes_t bytes_mod;
    bytes_mod.size = bytes.size;
    bytes_mod.data = bytes_buffer;
    return bytes_mod;
}

#endif  // MODULES_COMMON_CCSIMICS_DATA_ENCRYPTION_H_
