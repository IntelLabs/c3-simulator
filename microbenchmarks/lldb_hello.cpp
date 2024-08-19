// Copyright 2023-2024 Intel Corporation
// SPDX-License-Identifier: MIT

#include <malloc.h>
#include <unistd.h>
#include <climits>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include "malloc/cc_globals.h"

#define G_HELLO "Hello LLDB World!\n"
// #define G_HELLO "Howdy\n"

const char *g_hello = G_HELLO;

const bool allocate_on_stack = false;

template <typename T>
static inline std::string buf_to_hex_string(const T *buffer, size_t len) {
    const uint8_t *buf = reinterpret_cast<const uint8_t *>(buffer);
    std::stringstream ss;
    for (size_t i = 0; i < len; ++i) {
        ss << std::setw(2) << std::setfill('0') << std::hex
           << static_cast<int>(buf[i]);
    }
    return ss.str();
}

int main(int argc, char **argv) {
    const size_t str_size = sizeof(G_HELLO);
    auto *const original_str = reinterpret_cast<char *>(
            allocate_on_stack ? alloca(str_size) : malloc(str_size));
    auto *str = original_str;
    int end = str_size;

    printf("str allocated as %p (len: %lu, malloc_usable_size: %lu)\n", str,
           str_size, malloc_usable_size(str));

    strncpy(str, g_hello, str_size);
    printf("str content is: %s", str);

    auto *la_str = cc_dec_if_encoded_ptr<>(str);
    const auto slot_max_index = (uint64_t)is_encoded_cc_ptr(str)
                                        ? ca_get_inbound_offset(str, UINT_MAX)
                                        : INT_MAX;

    printf("str CA %p, corresponds to LA %p\n", str, la_str);
    printf("str CA has power-slot of size %lu, and maximum in-slot index: "
           "%lu\n",
           get_size_in_bytes((uint64_t)str), slot_max_index);

    printf("\nDoing byte-by-byte overwrite of str[0,end=%lu)\n..\n", str_size);

    // Corrupt end in ASM so compiler sees no evil.
    __asm__ __volatile__("mov %[len], (%[end])"
                         :
                         : [len] "r"(str_size * str_size), [end] "r"(&end)
                         : "memory");

    for (int i = 0; i < end; i++) {
        str++;
        if (i >= slot_max_index) {
            printf("Going to write to offset %d = address %p%s\n", i, str,
                   (i >= slot_max_index ? " (CA power-lost overflow!)" : ""));
        }
        *str = static_cast<char>('a' + argc);
    }

    printf("original_str still at %p, now with content: %s", original_str,
           original_str);

    if (!allocate_on_stack) {
        free(original_str);
    }
    return 0;
}
