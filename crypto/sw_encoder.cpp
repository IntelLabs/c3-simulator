/*
 Copyright 2024 Intel Corporation
 SPDX-License-Identifier: MIT
*/
#include "sw_encoder.h"    // NOLINT
#include "ascon_cipher.h"  // NOLINT
#include "bipbip.h"        // NOLINT

#include "ascon_cipher.cpp"  // NOLINT
#undef BITMASK
#include "bipbip.cpp"  // NOLINT

#include <assert.h>
#include <memory>

extern "C" {

std::shared_ptr<c3_support::SWEncoder<>> g_encoder;

void c3c_init_encoder(const uint8_t *data_key, const uint8_t *ptr_key) {
    g_encoder = std::make_shared<c3_support::SWEncoder<>>();

    if (data_key != nullptr) {
        g_encoder->set_data_key(data_key);
    }

    if (ptr_key != nullptr) {
        g_encoder->set_ptr_key(ptr_key);
    }
}

uint64_t c3c_decode_ptr(uint64_t p) {
    assert(g_encoder.get() != nullptr);
    return g_encoder->decode_ptr(p);
}

uint64_t c3c_encode_ptr(uint64_t p, uint64_t s, uint64_t v) {
    assert(g_encoder.get() != nullptr);
    return g_encoder->encode_ptr(p, s, v);
}

int c3c_encrypt_decrypt_bytes(const uint64_t p, uint8_t *b, size_t l) {
    assert(g_encoder.get() != nullptr);
    return g_encoder->encrypt_decrypt_bytes(p, b, l);
}

}  // extern "C"
