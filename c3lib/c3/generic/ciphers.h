// Copyright 2016-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef C3LIB_C3_GENERIC_CIPHERS_H_
#define C3LIB_C3_GENERIC_CIPHERS_H_

#include "c3/generic/defines.h"

// C3 ciphers
#define BIPBIP_CIPHER 1
#define ASCON_CIPHER 2
// #define K_CIPHER64 2

#define C3_KEY_SIZE(cipher)                                                    \
    (((cipher) == BIPBIP_CIPHER) ? 32 : ((cipher) == ASCON_CIPHER) ? 16 : -1)

#define KEY_SCHEDULE_SIZE(cipher) (((cipher) == ASCON_CIPHER) ? 16 : -1)

// Set default data and pointer ciphers
#ifndef CC_POINTER_CIPHER
#define CC_POINTER_CIPHER BIPBIP_CIPHER
#endif

#ifndef CC_DATA_CIPHER
#define CC_DATA_CIPHER ASCON_CIPHER
#endif

#define CIPHER_OFFSET_BITS 3

typedef uint8_t pointer_key_bytes_t[C3_KEY_SIZE(CC_POINTER_CIPHER)];
typedef struct {
    int size_;
    uint8_t bytes_[C3_KEY_SIZE(CC_POINTER_CIPHER)];
} pointer_key_t;

typedef uint8_t data_key_bytes_t[C3_KEY_SIZE(CC_DATA_CIPHER)];
typedef uint8_t data_key_schedule_t[KEY_SCHEDULE_SIZE(CC_DATA_CIPHER)];
typedef struct {
    int size_;
    uint8_t bytes_[C3_KEY_SIZE(CC_DATA_CIPHER)];
} data_key_t;

#endif  // C3LIB_C3_GENERIC_CIPHERS_H_
