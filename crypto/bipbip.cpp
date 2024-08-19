// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#include "bipbip.h"  // NOLINT
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

namespace crypto {

#define BITMASK(x) (0xFFFFFFFFFFFFFFFF >> (64 - x))

constexpr int KEY_SIZE_IN_BYTES = 32;

const uint8_t BBB[64] = {  // BipBipBox
        0x00, 0x01, 0x02, 0x03, 0x04, 0x06, 0x3e, 0x3c, 0x08, 0x11, 0x0e,
        0x17, 0x2b, 0x33, 0x35, 0x2d, 0x19, 0x1c, 0x09, 0x0c, 0x15, 0x13,
        0x3d, 0x3b, 0x31, 0x2c, 0x25, 0x38, 0x3a, 0x26, 0x36, 0x2a, 0x34,
        0x1d, 0x37, 0x1e, 0x30, 0x1a, 0x0b, 0x21, 0x2e, 0x1f, 0x29, 0x18,
        0x0f, 0x3f, 0x10, 0x20, 0x28, 0x05, 0x39, 0x14, 0x24, 0x0a, 0x0d,
        0x23, 0x12, 0x27, 0x07, 0x32, 0x1b, 0x2f, 0x16, 0x22};

const uint8_t PI1[24] = {1,  7,  6,  0,  2,  8,  12, 18, 19, 13, 14, 20,
                         21, 15, 16, 22, 23, 17, 9,  3,  4,  10, 11, 5};
const uint8_t PI2[24] = {0,  1,  4,  5,  8,  9,  2,  3,  6,  7,  10, 11,
                         16, 12, 13, 17, 20, 21, 15, 14, 18, 19, 22, 23};
const uint8_t PI3[24] = {16, 22, 11, 5, 2,  8,  0, 6, 19, 13, 12, 18,
                         14, 15, 1,  7, 21, 20, 4, 3, 17, 23, 10, 9};

const uint8_t PI1_inv[24] = {3, 0, 4,  19, 20, 23, 2, 1, 5,  18, 21, 22,
                             6, 9, 10, 13, 14, 17, 7, 8, 11, 12, 15, 16};
const uint8_t PI2_inv[24] = {0,  1,  6,  7,  2,  3,  8,  9,  4,  5,  10, 11,
                             13, 14, 19, 18, 12, 15, 20, 21, 16, 17, 22, 23};
const uint8_t PI3_inv[24] = {6,  14, 4,  19, 18, 3,  7,  15, 5,  23, 22, 2,
                             10, 9,  12, 13, 0,  20, 11, 8,  17, 16, 1,  21};

const uint8_t PI4[53] = {0,  13, 26, 39, 52, 12, 25, 38, 51, 11, 24, 37, 50, 10,
                         23, 36, 49, 9,  22, 35, 48, 8,  21, 34, 47, 7,  20, 33,
                         46, 6,  19, 32, 45, 5,  18, 31, 44, 4,  17, 30, 43, 3,
                         16, 29, 42, 2,  15, 28, 41, 1,  14, 27, 40};
const uint8_t PI5[53] = {0,  11, 22, 33, 44, 2,  13, 24, 35, 46, 4,  15, 26, 37,
                         48, 6,  17, 28, 39, 50, 8,  19, 30, 41, 52, 10, 21, 32,
                         43, 1,  12, 23, 34, 45, 3,  14, 25, 36, 47, 5,  16, 27,
                         38, 49, 7,  18, 29, 40, 51, 9,  20, 31, 42};

const uint8_t Sbox_table[64] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x06, 0x3e, 0x3c, 0x08, 0x11, 0x0e,
        0x17, 0x2b, 0x33, 0x35, 0x2d, 0x19, 0x1c, 0x09, 0x0c, 0x15, 0x13,
        0x3d, 0x3b, 0x31, 0x2c, 0x25, 0x38, 0x3a, 0x26, 0x36, 0x2a, 0x34,
        0x1d, 0x37, 0x1e, 0x30, 0x1a, 0x0b, 0x21, 0x2e, 0x1f, 0x29, 0x18,
        0x0f, 0x3f, 0x10, 0x20, 0x28, 0x05, 0x39, 0x14, 0x24, 0x0a, 0x0d,
        0x23, 0x12, 0x27, 0x07, 0x32, 0x1b, 0x2f, 0x16, 0x22};
const uint8_t Sbox_inv_table[64] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x31, 0x05, 0x3a, 0x08, 0x12, 0x35,
        0x26, 0x13, 0x36, 0x0a, 0x2c, 0x2e, 0x09, 0x38, 0x15, 0x33, 0x14,
        0x3e, 0x0b, 0x2b, 0x10, 0x25, 0x3c, 0x11, 0x21, 0x23, 0x29, 0x2f,
        0x27, 0x3f, 0x37, 0x34, 0x1a, 0x1d, 0x39, 0x30, 0x2a, 0x1f, 0x0c,
        0x19, 0x0f, 0x28, 0x3d, 0x24, 0x18, 0x3b, 0x0d, 0x20, 0x0e, 0x1e,
        0x22, 0x1b, 0x32, 0x1c, 0x17, 0x07, 0x16, 0x06, 0x2d};

static inline auto get_bit(uint64_t in, int position) {
    return (in >> position) & 0x1;
}

static inline block_word sbox(block_word state_in, const uint8_t table[64]) {
    block_word state_out = 0;
    for (int i = 3; i >= 0; i--) {
        state_out <<= 6;
        state_out |= (block_word)table[(state_in >> i * 6) & BITMASK(6)];
    }
    return state_out;
}

// S-box Layer S
static inline block_word SBL(block_word state_in) {
    return sbox(state_in, Sbox_table);
}

static inline block_word SBL_inv(block_word state_in) {
    return sbox(state_in, Sbox_inv_table);
}

// Linear Mixing Layer in Data Path: theta_d
static inline block_word LML1(block_word state_in) {
    state_in &= BITMASK(24);
    block_word state_in_shift2 = (state_in >> 2) | (state_in << (24 - 2));
    block_word state_in_shift12 = (state_in >> 12) | (state_in << (24 - 12));
    return (state_in ^ state_in_shift2 ^ state_in_shift12) & BITMASK(24);
}

static inline block_word LML1_inv(block_word state_in) {
    state_in &= BITMASK(24);
    block_word state_in_shift2 = (state_in >> 2) | (state_in << (24 - 2));
    block_word state_in_shift12 = (state_in >> 12) | (state_in << (24 - 12));
    block_word state_tmp =
            (state_in ^ state_in_shift2 ^ state_in_shift12) & BITMASK(24);
    return ((state_tmp >> 20) | (state_tmp << 4)) & BITMASK(24);
}

static tweak_word BPL(tweak_word state_in, const uint8_t PI[], int size) {
    tweak_word state_out = 0;
    for (int i = size - 1; i >= 0; i--) {
        state_out = state_out << 1;
        state_out |= get_bit(state_in, PI[i]);
    }
    return state_out;
}

static inline block_word KAD(block_word data, block_word tweak) {
    return data ^ tweak;
}

// Round Function: Core Round
static block_word RFC(block_word state_in) {
    block_word state = SBL(state_in);
    state = BPL((block_word)state, PI1, 24);
    state = LML1(state);
    return BPL((block_word)state, PI2, 24);
}

static block_word RFC_inv(block_word state_in) {
    block_word state = BPL((block_word)state_in, PI2_inv, 24);
    state = LML1_inv(state);
    state = BPL((block_word)state, PI1_inv, 24);
    return SBL_inv(state);
}

static inline block_word RFS(block_word in) {
    block_word tmp = SBL(in);
    return BPL((block_word)tmp, PI3, 24);
}
static inline block_word RFS_inv(block_word in) {
    block_word tmp = BPL((block_word)in, PI3_inv, 24);
    return SBL_inv(tmp);
}

// BipBip Encryption
static block_word BipBipEnc(block_word plaintext,
                            tweak_schedule_t *tweak_schedule) {
    block_word state = plaintext;
    state = KAD(state, tweak_schedule->tweak_round[11]);
    state = RFS_inv(state);
    state = KAD(state, tweak_schedule->tweak_round[10]);
    state = RFS_inv(state);
    state = KAD(state, tweak_schedule->tweak_round[9]);
    state = RFS_inv(state);
    state = KAD(state, tweak_schedule->tweak_round[8]);
    state = RFC_inv(state);
    state = KAD(state, tweak_schedule->tweak_round[7]);
    state = RFC_inv(state);
    state = KAD(state, tweak_schedule->tweak_round[6]);
    state = RFC_inv(state);
    state = KAD(state, tweak_schedule->tweak_round[5]);
    state = RFC_inv(state);
    state = KAD(state, tweak_schedule->tweak_round[4]);
    state = RFC_inv(state);
    state = KAD(state, tweak_schedule->tweak_round[3]);
    state = RFS_inv(state);
    state = KAD(state, tweak_schedule->tweak_round[2]);
    state = RFS_inv(state);
    state = KAD(state, tweak_schedule->tweak_round[1]);
    state = RFS_inv(state);
    state = KAD(state, tweak_schedule->tweak_round[0]);
    return state;
}

// BipBip Decryption
static block_word BipBipDec(block_word ciphertext,
                            tweak_schedule_t *tweak_schedule) {
    block_word state = ciphertext;
    state = KAD(state, tweak_schedule->tweak_round[0]);
    state = RFS(state);
    state = KAD(state, tweak_schedule->tweak_round[1]);
    state = RFS(state);
    state = KAD(state, tweak_schedule->tweak_round[2]);
    state = RFS(state);
    state = KAD(state, tweak_schedule->tweak_round[3]);
    state = RFC(state);
    state = KAD(state, tweak_schedule->tweak_round[4]);
    state = RFC(state);
    state = KAD(state, tweak_schedule->tweak_round[5]);
    state = RFC(state);
    state = KAD(state, tweak_schedule->tweak_round[6]);
    state = RFC(state);
    state = KAD(state, tweak_schedule->tweak_round[7]);
    state = RFC(state);
    state = KAD(state, tweak_schedule->tweak_round[8]);
    state = RFS(state);
    state = KAD(state, tweak_schedule->tweak_round[9]);
    state = RFS(state);
    state = KAD(state, tweak_schedule->tweak_round[10]);
    state = RFS(state);
    state = KAD(state, tweak_schedule->tweak_round[11]);
    return state;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Chi Layer
static inline tweak_word CHI(tweak_word state_in) {
    state_in &= BITMASK(53);
    tweak_word state_in_shift1 = (state_in >> 1) | (state_in << (53 - 1));
    tweak_word state_in_shift2 = (state_in >> 2) | (state_in << (53 - 2));
    return (state_in ^ (~state_in_shift1 & state_in_shift2)) & BITMASK(53);
}

// Linear Mixing Layer in Tweak Path: theta_t
static inline tweak_word LML2(tweak_word state_in) {
    state_in &= BITMASK(53);
    tweak_word state_in_shift1 = (state_in >> 1) | (state_in << (53 - 1));
    tweak_word state_in_shift2 = (state_in >> 8) | (state_in << (53 - 8));
    return (state_in ^ state_in_shift1 ^ state_in_shift2) & BITMASK(53);
}

// Linear Mixing Layer in Tweak Path: theta_p
static inline tweak_word LML3(tweak_word state_in) {
    state_in &= BITMASK(53);
    tweak_word state_in_shift1 = (state_in >> 1);
    return state_in ^ state_in_shift1;
}

static inline block_word deinterleave_odd_bits(tweak_word in) {
    in &= 0x555555555555;
    in = (in | (in >> 1)) & 0x3333333333333333;
    in = (in | (in >> 2)) & 0x0f0f0f0f0f0f0f0f;
    in = (in | (in >> 4)) & 0x00ff00ff00ff00ff;
    in = (in | (in >> 8)) & 0x0000ffff0000ffff;
    block_word out = (in | (in >> 16)) & 0xffffff;
    return out;
}

// Round Key Ectraction E0
static inline block_word RKE0(tweak_word in) {
    return deinterleave_odd_bits(in);
}
static inline block_word RKE1(tweak_word in) {
    return deinterleave_odd_bits(in >> 1);
}

// Round Function: G Round
static tweak_word RGC(tweak_word state) {
    state = BPL(state, PI4, 53);
    state = LML2(state);
    state = BPL(state, PI5, 53);
    state = CHI(state);
    return state;
}

// Round Function: G' Round
static tweak_word RGP(tweak_word state) {
    state = BPL(state, PI4, 53);
    state = LML3(state);
    state = BPL(state, PI5, 53);
    state = CHI(state);
    return state;
}

// BipBip Tweak Schedule
static tweak_schedule_t TwkSc(tweak_word tweak_padded,
                              key_schedule_t *key_schedule) {
    tweak_schedule_t tweak_schedule;
    tweak_schedule.tweak_round[0] = key_schedule->data_round_key;
    tweak_padded ^= key_schedule->tweak_round_key[0];
    tweak_padded = CHI(tweak_padded);
    tweak_schedule.tweak_round[1] = RKE0(tweak_padded);
    tweak_schedule.tweak_round[2] = RKE1(tweak_padded);
    tweak_padded ^= key_schedule->tweak_round_key[1];
    tweak_padded = RGC(tweak_padded);
    tweak_schedule.tweak_round[3] = RKE0(tweak_padded);
    tweak_schedule.tweak_round[4] = RKE1(tweak_padded);
    tweak_padded ^= key_schedule->tweak_round_key[2];
    tweak_padded = RGC(tweak_padded);
    tweak_padded = RGP(tweak_padded);
    tweak_schedule.tweak_round[5] = RKE0(tweak_padded);
    tweak_padded ^= key_schedule->tweak_round_key[3];
    tweak_padded = RGC(tweak_padded);
    tweak_schedule.tweak_round[6] = RKE0(tweak_padded);
    tweak_padded = RGP(tweak_padded);
    tweak_schedule.tweak_round[7] = RKE0(tweak_padded);
    tweak_padded ^= key_schedule->tweak_round_key[4];
    tweak_padded = RGC(tweak_padded);
    tweak_schedule.tweak_round[8] = RKE0(tweak_padded);
    tweak_padded = RGP(tweak_padded);
    tweak_schedule.tweak_round[9] = RKE0(tweak_padded);
    tweak_padded ^= key_schedule->tweak_round_key[5];
    tweak_padded = RGC(tweak_padded);
    tweak_schedule.tweak_round[10] = RKE0(tweak_padded);
    tweak_schedule.tweak_round[11] = RKE1(tweak_padded);
    return tweak_schedule;
}

// Initializing
static tweak_word pad_tweak(tweak_word tweak) {
    tweak_word tweak_padded = tweak & BITMASK(40);
    tweak_padded <<= 1;
    tweak_padded |= 0x1;
    tweak_padded <<= 12;
    return tweak_padded;
}

static key_schedule_t GenKeySchedule(const tweak_word MK[4]) {
    key_schedule_t key_schedule;
    key_schedule.data_round_key = 0;
    int position = 1;
    for (int i = 0; i < 24; i++) {
        position = (position * 3) % 256;  //
        block_word bit = get_bit(MK[position / 64], position % 64);
        key_schedule.data_round_key |= (bit << i);
    }

    position = 53;
    for (int i = 0; i < 6; i++) {
        key_schedule.tweak_round_key[i] = 0;
        for (int j = 0; j < 53; j++) {
            tweak_word bit = get_bit(MK[position / 64], position % 64);
            position = (position + 1) % 256;
            key_schedule.tweak_round_key[i] |= (bit << j);
        }
    }
    return key_schedule;
}

PointerCipher24b::PointerCipher24b() { key_initialized = false; }

PointerCipher24b::PointerCipher24b(const uint8_t *key, int key_size) {
    init_key(key, key_size);
}

void PointerCipher24b::init_key(const uint8_t *key, int key_size) {
    assert(key_size == KEY_SIZE_IN_BYTES);
    key_schedule = GenKeySchedule(reinterpret_cast<const uint64_t *>(key));
    tweak = 0;
    tweak_schedule = TwkSc(pad_tweak(tweak), &key_schedule);
    previous_decryption_pair.ciphertext_in = 0;
    previous_decryption_pair.tweak = tweak;
    previous_decryption_pair.plaintext_out =
            BipBipDec(previous_decryption_pair.ciphertext_in, &tweak_schedule);
    key_initialized = true;
}
void PointerCipher24b::regenerate_tweak_schedule_for_new_tweak(
        tweak_word new_tweak) {
    if (new_tweak != this->tweak) {
        this->tweak = new_tweak;
        uint64_t padded_tweak = pad_tweak(new_tweak);
        tweak_schedule = TwkSc(padded_tweak, &key_schedule);
    }
}

uint32_t PointerCipher24b::encrypt(uint32_t plaintext, uint64_t tweak) {
    assert(key_initialized);
    regenerate_tweak_schedule_for_new_tweak(tweak);
    return BipBipEnc(plaintext, &tweak_schedule);
}

uint32_t PointerCipher24b::decrypt(uint32_t ciphertext, uint64_t tweak) {
    assert(key_initialized);
    regenerate_tweak_schedule_for_new_tweak(tweak);
    uint32_t plain = BipBipDec(ciphertext, &tweak_schedule);
    return plain;
    // if (previous_decryption_pair.ciphertext_in == ciphertext &&
    // previous_decryption_pair.tweak == tweak) {
    //     SIM_printf("plain=%08x\n", previous_decryption_pair.plaintext_out);
    //     return previous_decryption_pair.plaintext_out; // same inputs as
    //     previous invocation
    // } else {
    //     previous_decryption_pair.ciphertext_in = ciphertext;
    //     previous_decryption_pair.tweak = tweak;
    //     previous_decryption_pair.plaintext_out = BipBipDec(ciphertext,
    //     &tweak_schedule); SIM_printf("plain=%08x\n",
    //     previous_decryption_pair.plaintext_out); return
    //     previous_decryption_pair.plaintext_out;
    // }
}

}  // namespace crypto
