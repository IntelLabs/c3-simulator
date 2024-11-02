// Copyright 2019-2024 Intel Corporation
// SPDX-License-Identifier: MIT

/**
 * File: encodings.h
 *
 * Description: Structs and other encodings used in cryptographic computing
 * models.
 *
 * Original Authors: Andrew Weiler, Sergej Deutsch
 */

#ifndef CRYPTO_CC_ENCODING_H_
#define CRYPTO_CC_ENCODING_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "ascon_cipher.h"  // NOLINT
#include "bipbip.h"        // NOLINT
#include "c3/generic/ca.h"
#include "c3/generic/defines.h"

#if defined(__cplusplus)

static constexpr size_t kC3PtrKeySize = C3_KEY_SIZE(CC_POINTER_CIPHER);
static constexpr size_t kC3DataKeySize = C3_KEY_SIZE(CC_DATA_CIPHER);
static constexpr uint8_t kC3DefaultPtrKey[kC3PtrKeySize] = {
        0xd1, 0xbe, 0x2c, 0xdb, 0xb5, 0x82, 0x4d, 0x03, 0x17, 0x5c, 0x25,
        0x2a, 0x20, 0xb6, 0xf2, 0x93, 0xfd, 0x01, 0x96, 0xe7, 0xb5, 0xe6,
        0x88, 0x1c, 0xb3, 0x69, 0x22, 0x60, 0x38, 0x09, 0xf6, 0x68};
static constexpr uint8_t kC3DefaultDataKey[kC3DataKeySize] = {
        0xb5, 0x82, 0x4d, 0x03, 0x17, 0x5c, 0x25, 0x2a,
        0xfc, 0x71, 0x1e, 0x01, 0x02, 0x60, 0x87, 0x91};

class SimicsConnection;

/**
 * @brief Extracts the tweak bits from a CA
 *
 * @param pointer
 * @param ptr_metadata
 * @return uint64_t
 */
static inline uint64_t generate_tweak(uint64_t pointer,
                                      ptr_metadata_t *ptr_metadata) {
    uint64_t tweak = 0;
    tweak = (uint64_t)convert_to_cc_ptr(&pointer)->plaintext_;
    tweak &= get_tweak_mask(ptr_metadata->size_);  // mask off mutable bits
    tweak |= ((uint64_t)ptr_metadata->size_) << PLAINTEXT_SIZE;
    return tweak;
}

/**
 * @brief Get pointer metadata for given CA
 *
 * @param pointer
 * @return ptr_metadata_t
 */
static inline ptr_metadata_t get_pointer_metadata(uint64_t pointer) {
    const auto size = static_cast<unsigned char>(ca_get_size(pointer));
    ptr_metadata_t metadata = {0};
    metadata.size_ = size;
    metadata.adjust_ = (size == SPECIAL_SIZE_ENCODING_WITH_ADJUST) ? 0x1 : 0x0;
    return metadata;
}

/**
 * @brief Implements pointer encoding

 * This implements the encoding and decoding of C3 encoded addresses, i.e.,
 * cryptographic addresses (CAs) from/to canonical linear addresses.
 *
 * The object will be used in various callbacks to internally, and is also used
 * in the implementation of the callbacks that implements the C3 ISA extensions
 * that provide encptr and decptr instructions. It may internally cache key
 * configurations, and key updates must be propagated here by invoking
 * init_pointer_key().
 */
class CCPointerEncodingBase {
 protected:
    crypto::PointerCipher24b pointer_cipher_;

 public:
    CCPointerEncodingBase() = default;
    virtual inline ~CCPointerEncodingBase() = default;

    /**
     * @brief Encrypt an already decorated CA
     *
     * Expects the CA to already be decorated with any necessary metadata and
     * will only perform encryption of the CA.
     *
     * @param ptr
     * @param md
     * @return ca_t
     */
    virtual inline ca_t encrypt_ptr(ca_t ptr, ptr_metadata_t *md) {
        uint32_t ciphertext = pointer_cipher_.encrypt(
                get_ciphertext(ptr.uint64_), generate_tweak(ptr.uint64_, md));
        ptr.ciphertext_low_ = ciphertext;
        ptr.ciphertext_high_ = ciphertext >> CIPHERTEXT_LOW_SIZE;
        return ptr;
    }

    /**
     * @brief Decrypt a CA to a decorated linear address
     *
     * Decrypt a CA, but does not remove additional metadata from the resulting
     * decorated pointer.
     *
     * @param ptr
     * @return ca_t
     */
    virtual inline ca_t decrypt_ptr(ca_t ptr) {
        ptr_metadata_t md = get_pointer_metadata(ptr.uint64_);

        uint32_t plaintext = pointer_cipher_.decrypt(
                get_ciphertext(ptr.uint64_), generate_tweak(ptr.uint64_, &md));

        ptr.ciphertext_low_ = plaintext;
        ptr.ciphertext_high_ = plaintext >> CIPHERTEXT_LOW_SIZE;
        return ptr;
    }

    /**
     * @brief Decorate, i.e., add metadata into a give LA
     *
     * The decorated pointer will be unusable as-is, and is expected to next
     * be encrypted.
     *
     * @param ptr
     * @param md
     * @return ca_t
     */
    virtual inline ca_t decorate_ptr(ca_t ptr, ptr_metadata_t *md) {
        ptr.version_ = md->version_;
        ptr.enc_size_ = md->size_;
        return ptr;
    }

    /**
     * @brief Remove additional metadata from an already decrypted CA
     *
     * @param ptr
     * @return ca_t
     */
    virtual inline ca_t undecorate_ptr(ca_t ptr) {
        ptr.s_extended_ = (ptr.s_prime_bit_) != 0u ? C3_FMASK : 0x0;
        return ptr;
    }

    /**
     * @brief Initialize a new pointer key
     *
     * This will configure a new pointer key for encrypting and decrypting
     * CAs.
     *
     * @param key
     * @param key_size
     */
    inline void init_pointer_key(const uint8_t *key, int key_size) {
        pointer_cipher_.init_key(key, key_size);
    }

    /**
     * @brief Decodes a given CA to an LA
     *
     * Will first decrypt the given CA, and then undecorate it before returning
     * the original canonical linear address (unless the CA was corrupted).
     *
     * @param encoded_pointer
     * @return uint64_t
     */
    virtual inline uint64_t decode_pointer(uint64_t encoded_pointer) {
        return undecorate_ptr(decrypt_ptr(get_ca_t(encoded_pointer))).uint64_;
    }

    inline uint64_t decode_pointer_if_encoded(uint64_t ptr) {
        return is_encoded_cc_ptr(ptr) ? decode_pointer(ptr) : ptr;
    }

    /**
     * @brief Encodes g given LA to a CA
     *
     * This will first decorate the LA with additional metadata as needed, and
     * the encrypt the LA to produce and return  a valid CA.
     *
     * @param pointer
     * @param ptr_metadata
     * @return uint64_t
     */
    virtual inline uint64_t encode_pointer(uint64_t pointer,
                                           ptr_metadata_t *ptr_metadata) {
        return encrypt_ptr(decorate_ptr(get_ca_t(pointer), ptr_metadata),
                           ptr_metadata)
                .uint64_;
    }
};

/**
 * @brief Exact copy of CCPointerEncoding but marked final
 *
 * May avoid some bugs in bare-bones testing environments without loader
 * support, e.g., by avoiding the need for virtual call tables and such.
 */
class CCPointerEncodingPlain final : public CCPointerEncodingBase {
 public:
    CCPointerEncodingPlain() {}
};

class CCPointerEncoding final : public CCPointerEncodingBase {
 public:
    CCPointerEncoding() : CCPointerEncodingBase() {}
    CCPointerEncoding(SimicsConnection *con) : CCPointerEncodingBase() {}
};

class CCDataEncryption {
 private:
    data_key_t data_key_ = {
            0,    // size_
            {0},  // bytes_
    };

 public:
    inline CCDataEncryption() {
        data_key_.size_ = kC3DataKeySize;
        set_key(kC3DefaultDataKey);
    }

    inline void set_key(const uint8_t *key) {
        memcpy(data_key_.bytes_, key, kC3DataKeySize);
    }

    inline void encrypt_decrypt_bytes(uint64_t la, uint8_t *const input,
                                      uint8_t *output, size_t size) {
        CCDataEncryption::encrypt_decrypt_bytes(la, &data_key_, input, output,
                                                size);
    }

 public:
    static inline void get_countermode_mask(uint64_t la,
                                            const data_key_t *data_key,
                                            size_t size, uint8_t *mask) {
        uint64_t *mask64 = reinterpret_cast<uint64_t *>(mask);
        uint64_t la_base = (la >> CIPHER_OFFSET_BITS) << CIPHER_OFFSET_BITS;
        uint64_t la_end = la + static_cast<uint64_t>(size);

        while (la_base < la_end) {
            *mask64 = ascon64b_stream(la_base, data_key->bytes_);
            mask64++;
            la_base += (0x1ULL << CIPHER_OFFSET_BITS);
        }
    }

    static inline uint8_t *encrypt_decrypt_bytes(uint64_t la_encoded,
                                                 const data_key_t *data_key,
                                                 const uint8_t *input,
                                                 uint8_t *output, size_t size) {
        uint8_t countermode_mask_unaligned[64 + 8];
        get_countermode_mask(la_encoded, data_key, size,
                             countermode_mask_unaligned);

        int offset = static_cast<int>(la_encoded & 0x7);
        for (int i = size - 1; i >= 0; i--) {
            output[i] = input[i] ^ countermode_mask_unaligned[i + offset];
        }

        return output;
    }

    static inline uint8_t *
    encrypt_decrypt_many_bytes(uint64_t la_encoded, const data_key_t *data_key,
                               const uint8_t *input, uint8_t *output,
                               size_t size) {
        constexpr size_t block_size = 64;

        for (unsigned i = 0; size > i; i += block_size) {
            const auto chunk_ptr = la_encoded + i;
            const auto chunk_size =
                    (size - i) > block_size ? block_size : (size - i);
            const auto in = input + i;
            const auto out = output + i;

            encrypt_decrypt_bytes(chunk_ptr, data_key, in, out, chunk_size);
        }

        return output;
    }
};

#endif
#endif  // CRYPTO_CC_ENCODING_H_
