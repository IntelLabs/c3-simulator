// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#pragma once
#include <stdint.h>

namespace crypto {

#define tweak_word uint64_t
#define block_word uint32_t

typedef struct {
    block_word data_round_key;
    tweak_word tweak_round_key[6];
} key_schedule_t;

typedef struct {
    block_word tweak_round[12];
} tweak_schedule_t;

typedef struct {
    tweak_word tweak;
    block_word ciphertext_in;
    block_word plaintext_out;
} decryption_pair_t;

class PointerCipher24b final {
 public:
    PointerCipher24b();
    PointerCipher24b(const uint8_t *key, int key_size);
    void init_key(const uint8_t *key, int key_size);
    uint32_t encrypt(uint32_t plaintext, uint64_t tweak);
    uint32_t decrypt(uint32_t ciphertext, uint64_t tweak);

 private:
    key_schedule_t key_schedule;
    tweak_word tweak;
    tweak_schedule_t tweak_schedule;
    bool key_initialized;
    decryption_pair_t previous_decryption_pair;
    void regenerate_tweak_schedule_for_new_tweak(tweak_word new_tweak);
};
}  // namespace crypto
