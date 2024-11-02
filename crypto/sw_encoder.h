// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef CRYPTO_SW_ENCODER_H_
#define CRYPTO_SW_ENCODER_H_

#if defined(__cplusplus)

#include <stdarg.h>
#include <stdio.h>
#include <algorithm>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include "cc_encoding.h"  // NOLINT

namespace c3_support {

static constexpr uint64_t bad_ca = UINT64_MAX;

static inline std::string buf_to_hex_string(const uint8_t *buf, size_t len) {
    std::stringstream ss;
    for (size_t i = 0; i < len; ++i) {
        ss << std::setw(2) << std::setfill('0') << std::hex
           << static_cast<int>(buf[i]);
    }
    return ss.str();
}

/**
 * @brief
 *
 * @param buf key buffer
 * @param chars string
 * @param len length of string
 */
static inline void string_to_hex_buf(uint8_t *buf, const char *chars,
                                     size_t len) {
    const std::string str = chars;

    for (size_t i = 0; i < len; i += 2) {
        buf[i / 2] = (uint8_t)strtol(str.substr(i, 2).c_str(), NULL, 16);
    }
}

static inline uint64_t hex_string_to_uint64(const char *str) {
    uint64_t num;
    std::stringstream ss;
    ss << std::hex << str;
    ss >> num;
    return num;
}

static inline void default_log(const char *format, ...) {
    va_list arglist;
    va_start(arglist, format);
    std::string str_format(format);
    str_format.append("\n");
    vfprintf(stderr, str_format.c_str(), arglist);
    va_end(arglist);
}

template <void (*log)(const char *format, ...) = c3_support::default_log>
class SWEncoder {
 public:
    SWEncoder()
        : dataenc(std::make_shared<CCDataEncryption>()),
          ptrenc(std::make_shared<CCPointerEncoding>()) {
        set_ptr_key(kC3DefaultPtrKey);
        set_data_key(kC3DefaultDataKey);
    }

    ~SWEncoder() = default;

    const uint8_t *get_data_key() const {
        return (const uint8_t *)&c3_data_key;
    }
    const uint8_t *get_ptr_key() const { return (const uint8_t *)&c3_ptr_key; }

    const std::string get_data_key_str() const {
        return buf_to_hex_string(get_data_key(), kC3DataKeySize);
    }

    const std::string get_ptr_key_str() const {
        return buf_to_hex_string(get_ptr_key(), kC3PtrKeySize);
    }

    void set_data_key(const uint8_t *key) {
        memcpy(c3_data_key, key, kC3DataKeySize);
        dataenc->set_key(c3_data_key);
        log("Setting data key to %s",
            buf_to_hex_string(c3_data_key, kC3DataKeySize).c_str());
    }

    void set_ptr_key(const uint8_t *key) {
        memcpy(c3_ptr_key, key, kC3PtrKeySize);
        ptrenc->init_pointer_key(c3_ptr_key, kC3PtrKeySize);
        log("Setting ptr key to %s",
            buf_to_hex_string(c3_ptr_key, kC3PtrKeySize).c_str());
    }

    uint64_t decode_ptr(uint64_t ptr) {
        assert(is_encoded_cc_ptr(ptr));
        uint64_t new_ptr = ptrenc->decode_pointer(ptr);
        log("Decoding CA: 0x%016lx -> 0x%016lx", ptr, new_ptr);
        return new_ptr;
    }

    uint64_t decode_ptr_if_ca(const uint64_t ptr) {
        return is_encoded_cc_ptr(ptr) ? decode_ptr(ptr) : ptr;
    }

    uint64_t encode_ptr(const uint64_t ptr, const uint64_t size,
                        const uint64_t version) {
        assert(!is_encoded_cc_ptr(ptr));

        if (!cc_can_box(ptr, size)) {
            log("Cannot box 0x%016lx to CA-slot of size %ld, skipping pointer "
                "encoding",
                ptr, size);
            return ptr;
        }

        ptr_metadata_t md;
        try_box(ptr, size, &md);
        md.version_ = version;

        const uint64_t new_ptr = ptrenc->encode_pointer(ptr, &md);
        log("Encoding LA: 0x%016lx -> 0x%016lx", ptr, new_ptr);
        return new_ptr;
    }

    template <typename T>
    T *encode_ptr(const T *p, const uint64_t s, const uint64_t v) {
        return reinterpret_cast<T *>(
                encode_ptr(reinterpret_cast<uint64_t>(p), s, v));
    }

    bool encrypt_decrypt_bytes(const uint64_t ca, uint8_t *buf, size_t len) {
        if (!is_encoded_cc_ptr(ca)) {
            return false;
        }
        const auto mask = get_tweak_mask(ca);
        const auto masked_addr = mask ^ ca;
        const auto max_size = (1UL + ~mask) - masked_addr;
        const auto bytes_in_ps = std::min(len, max_size);

        auto *tmp_buf = reinterpret_cast<uint8_t *>(malloc(len));
        memcpy(tmp_buf, buf, len);
        dataenc->encrypt_decrypt_bytes(ca, tmp_buf, buf, bytes_in_ps);
        log("Decrypted %lu bytes at %p (tweak: 0x%016lx)\n\t\t   %s\n\t\t-> %s",
            bytes_in_ps, reinterpret_cast<void *>(decode_ptr(ca)), ca,
            buf_to_hex_string(tmp_buf, bytes_in_ps).c_str(),
            buf_to_hex_string(buf, bytes_in_ps).c_str());

        free(tmp_buf);
        return true;
    }

 private:
    uint8_t c3_ptr_key[kC3PtrKeySize];
    uint8_t c3_data_key[kC3DataKeySize];

    std::shared_ptr<CCDataEncryption> dataenc;
    std::shared_ptr<CCPointerEncoding> ptrenc;
};

};  // namespace c3_support

extern "C" {
#endif  // defined(__cplusplus)

void c3c_init_encoder(const uint8_t *data_key, const uint8_t *ptr_key);
uint64_t c3c_decode_ptr(uint64_t p);
uint64_t c3c_encode_ptr(uint64_t p, uint64_t s, uint64_t v);
int c3c_encrypt_decrypt_bytes(const uint64_t p, uint8_t *b, size_t l);

#if defined(__cplusplus)
}  // extern "C"
#endif  // defined(__cplusplus)

#endif  // CRYPTO_SW_ENCODER_H_
