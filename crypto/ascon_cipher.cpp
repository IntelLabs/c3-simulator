/*
 Copyright 2023 Intel Corporation
 SPDX-License-Identifier: MIT
*/
#include "ascon_cipher.h"  // NOLINT
#include <stdint.h>

#define CRYPTO_KEYBYTES 16
#define CRYPTO_BYTES 16
#define CRYPTO_NOOVERLAP 1
#define ASCON_PRF_BYTES 0
#define ASCON_PRF_ROUNDS 12
#define ASCON_PRF_OUT_RATE 16
#define ASCON_PRF_IN_RATE 32
#define ASCON_PRFA_IN_RATE 40
#define ASCON_PRF_IV 0x80808c0000000000ull
#define ASCON_PRFA_IV 0x80808c0400000000ull
#define ASCON_MAC_IV 0x80808c0000000080ull
#define ASCON_MACA_IV 0x80808c0400000080ull
#define S_X0_INIT 0x80808c0000000000ull

#define RC0 0xf0
#define RC1 0xe1
#define RC2 0xd2
#define RC3 0xc3
#define RC4 0xb4
#define RC5 0xa5
#define RC6 0x96
#define RC7 0x87
#define RC8 0x78
#define RC9 0x69
#define RCa 0x5a
#define RCb 0x4b

typedef union {
    uint64_t x[5];
    uint32_t w[5][2];
    uint8_t b[5][8];
} ascon_state_t;

static inline uint64_t ROR(uint64_t x, int n) {
    return x >> n | x << (-n & 63);
}

static inline void ROUND(ascon_state_t *s, uint8_t C) {
    ascon_state_t t;
    /* round constant */
    s->x[2] ^= C;
    /* s-box layer */
    s->x[0] ^= s->x[4];
    s->x[4] ^= s->x[3];
    s->x[2] ^= s->x[1];
    t.x[0] = s->x[0] ^ (~s->x[1] & s->x[2]);
    t.x[2] = s->x[2] ^ (~s->x[3] & s->x[4]);
    t.x[4] = s->x[4] ^ (~s->x[0] & s->x[1]);
    t.x[1] = s->x[1] ^ (~s->x[2] & s->x[3]);
    t.x[3] = s->x[3] ^ (~s->x[4] & s->x[0]);
    t.x[1] ^= t.x[0];
    t.x[3] ^= t.x[2];
    t.x[0] ^= t.x[4];
    /* linear layer */
    s->x[2] = t.x[2] ^ ROR(t.x[2], 6 - 1);
    s->x[3] = t.x[3] ^ ROR(t.x[3], 17 - 10);
    s->x[4] = t.x[4] ^ ROR(t.x[4], 41 - 7);
    s->x[0] = t.x[0] ^ ROR(t.x[0], 28 - 19);
    s->x[1] = t.x[1] ^ ROR(t.x[1], 61 - 39);
    s->x[2] = t.x[2] ^ ROR(s->x[2], 1);
    s->x[3] = t.x[3] ^ ROR(s->x[3], 10);
    s->x[4] = t.x[4] ^ ROR(s->x[4], 7);
    s->x[0] = t.x[0] ^ ROR(s->x[0], 19);
    s->x[1] = t.x[1] ^ ROR(s->x[1], 39);
    s->x[2] = ~s->x[2];
}

static inline void P12ROUNDS(ascon_state_t *s) {
    ROUND(s, RC0);
    ROUND(s, RC1);
    ROUND(s, RC2);
    ROUND(s, RC3);
    ROUND(s, RC4);
    ROUND(s, RC5);
    ROUND(s, RC6);
    ROUND(s, RC7);
    ROUND(s, RC8);
    ROUND(s, RC9);
    ROUND(s, RCa);
    ROUND(s, RCb);
}

static inline uint64_t PAD(int i) { return 0x80ull << (56 - 8 * i); }

// int crypto_prf(unsigned char* out, const unsigned char* in, const unsigned
// char* k) {
uint64_t ascon64b_stream(uint64_t data_in, const uint8_t *key) {
    /* load key */
    const uint64_t K0 = *reinterpret_cast<const uint64_t *>(&key[0]);
    const uint64_t K1 = *reinterpret_cast<const uint64_t *>(&key[8]);
    /* initialize */
    ascon_state_t s;
    s.x[0] = S_X0_INIT;
    s.x[1] = K0;
    s.x[2] = K1;
    s.x[3] = 0;
    s.x[4] = 0;

    P12ROUNDS(&s);

    /* absorb full plaintext words */
    s.x[0] ^= data_in;
    s.x[1] ^= PAD(0);
    /* domain separation */
    s.x[4] ^= 1;

    /* squeeze */
    P12ROUNDS(&s);
    return s.x[0];
}
