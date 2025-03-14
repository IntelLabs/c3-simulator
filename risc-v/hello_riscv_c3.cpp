// Copyright 2024-2025 Intel Corporation
// SPDX-License-Identifier: MIT

// Based on copy from microbenchmarks/sw_encoder_test.cpp

#include "c3/crypto/cc_encoding.h"
#include "c3/crypto/sw_encoder.h"

// The c3lib definition of MAGIC only has the x86-specific implementation
#undef MAGIC
#include "simics/magic-instruction.h"

#include <unistd.h>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>

#define str "Hello World!"
#define str_len sizeof(str)

static inline void print_byte_by_byte(const char *buffer, size_t len,
                                      const char *name) {
    std::cerr << "Printing " << len << " chars " << " byte-by-byte via " << name
              << " " << std::setw(16) << std::setfill('0') << std::hex
              << reinterpret_cast<uintptr_t>(buffer) << std::dec << ": ";
    for (auto i = 0UL; i < len; i++) {
        std::cerr << buffer[i];
    }
    std::cerr << std::endl;

    std::cerr << "Hex, byte-by-byte: ";
    for (auto i = 0UL; i < len; i++) {
        uint8_t byte = static_cast<uint8_t>(buffer[i]);
        std::cerr << std::setw(2) << std::setfill('0') << std::hex
                  << static_cast<int>(byte) << std::dec << " ";
    }
    std::cerr << std::endl;
}

static inline void copy_byte_by_byte(char *dst, const char *src, size_t len) {
    for (auto i = 0UL; i < len; i++) {
        dst[i] = src[i];
    }
}

static inline void sleep_magic() {
    // Sleep a bit here to make sure SSH passes on prior output before break

    sleep(1);
    // MAGIC(0);
}

int main() {
    const size_t buffer_size = 256;
    char *buffer = new char[buffer_size];
    char *la = buffer;

    memset(la, 0, buffer_size);
    memcpy(la, str, str_len);
    std::cerr << "Buffer address: " << static_cast<void *>(la) << std::endl;
    std::cerr << "       content: " << la << std::endl;
    std::cerr << std::endl;

    std::cerr << "Creating encoder..." << std::endl;
    auto encoder = std::make_shared<c3_support::SWEncoder<>>();
    std::cerr << std::endl;

    std::cerr << "Encoding CA..." << std::endl;
    char *ca = encoder->encode_ptr(la, buffer_size, 0);
    std::cerr << std::endl;

    // NOTE: Print byte-by-byte to avoid overflows (e.g., if garbled decryption
    //       causes the string-terminator to not be detected).
    print_byte_by_byte(la, str_len, "la");
    print_byte_by_byte(ca, str_len, "ca");
    std::cerr << std::endl;

    std::cerr << "Reading ca[0] = 'A' (i.e 0x41)..." << std::endl;
    sleep_magic();
    fprintf(stderr, "ca[0]: %c\n", ca[0]);
    fprintf(stderr, "la[0]: %c\n", la[0]);
    std::cerr << std::endl;

    std::cerr << "Writing ca[0] = 'A'..." << std::endl;
    ca[0] = 'A';
    std::cerr << std::endl;

    std::cerr << "Reading ca[0] = 'A'..." << std::endl;
    sleep_magic();
    fprintf(stderr, "ca[0]: %c\n", ca[0]);
    fprintf(stderr, "la[0]: %c\n", la[0]);
    std::cerr << std::endl;

    std::cerr << "Rewriting some chars byte-by-byte... ." << std::endl;
    sleep_magic();
    ca[0] = 'H';
    ca[1] = 'e';
    ca[2] = 'l';
    ca[3] = 'l';
    ca[4] = 'o';
    ca[5] = ' ';
    ca[6] = 'c';
    ca[7] = '3';
    ca[8] = '\0';
    std::cerr << std::endl;

    std::cerr << "Printing some chars byte-by-byte... ." << std::endl;
    sleep_magic();
    fprintf(stderr, "0: 0x%x\n", static_cast<int>(ca[0]));
    fprintf(stderr, "1: 0x%x\n", static_cast<int>(ca[1]));
    fprintf(stderr, "2: 0x%x\n", static_cast<int>(ca[2]));
    fprintf(stderr, "3: 0x%x\n", static_cast<int>(ca[3]));
    fprintf(stderr, "4: 0x%x\n", static_cast<int>(ca[4]));
    fprintf(stderr, "5: 0x%x\n", static_cast<int>(ca[5]));
    fprintf(stderr, "6: 0x%x\n", static_cast<int>(ca[6]));
    fprintf(stderr, "7: 0x%x\n", static_cast<int>(ca[7]));
    fprintf(stderr, "8: 0x%x\n", static_cast<int>(ca[7]));
    std::cerr << std::endl;
    print_byte_by_byte(ca, 9, "ca");

    std::cerr << "Print the whole str_len " << str_len << " chars..."
              << std::endl;
    sleep_magic();
    print_byte_by_byte(la, str_len, "la");
    print_byte_by_byte(ca, str_len, "ca");
    std::cerr << std::endl;

    std::cerr << "Rewriting buffer via CA..." << std::endl;
    // memset(ca, 0, buffer_size);
    memcpy(ca, str, str_len);
    ca[str_len] = '\0';
    sleep_magic();
    std::cerr << "Buffer address:   " << static_cast<void *>(ca) << std::endl;
    sleep_magic();

    // FIXME: printf fails
    // fprintf(stderr, " fprintf content: %s\n", ca);
    // sleep_magic();

    // FIXME: The C++print routines here fail
    // std::cerr << "  stdout content: " << ca << std::endl;
    // sleep_magic();

    delete[] buffer;
    return 0;
}
