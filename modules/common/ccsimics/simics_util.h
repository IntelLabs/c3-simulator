/*
 Copyright 2016 Intel Corporation
 SPDX-License-Identifier: MIT
*/
#ifndef MODULES_COMMON_CCSIMICS_SIMICS_UTIL_H_
#define MODULES_COMMON_CCSIMICS_SIMICS_UTIL_H_

#include <sstream>
#include <string>
#include <simics/simulator/output.h>

#if defined(__cplusplus)
extern "C" {
#endif  // defined(__cplusplus)

#define dbgprint(f, ...)                                                       \
    SIM_printf("(%s:%d) %s: " f "\n", __FILE__, __LINE__, __func__,            \
               ##__VA_ARGS__)

#define ifdbgprint(pred, ...)                                                  \
    if (pred)                                                                  \
    dbgprint(__VA_ARGS__)

#define sizeof_field(t, f) sizeof((static_cast<t *>(0))->f)  // NOLINT

static inline bool is_power_of_2(size_t n) { return ((n & (n - 1)) == 0); }

static inline void print_bytes(const char *name, const void *buffer,
                               size_t size) {
    SIM_printf("\t %s (%lu) = ", name, size);
    for (int i = size - 1; i >= 0; i--) {
        SIM_printf("%02x ", *((uint8_t *)buffer + i));  // NOLINT
    }
    SIM_printf("\n");
}

/**
 * @brief Convert uint8_t array to a string in hexadecimal
 *
 * @param buf
 * @param len
 * @return std::string
 */
static inline std::string buf_to_hex_string(const uint8_t *buf, size_t len) {
    std::stringstream ss;
    for (size_t i = 0; i < len; ++i) {
        ss << std::hex << static_cast<int>(buf[i]);
    }
    return ss.str();
}

/**
 * @brief Helper function for setting common ASAN options
 *
 * ASAN is not used by default, but if used, it will look for a function:
 *      const char *__asan_default_options()
 * to fetch program options.
 *
 * @return const char*
 */
static inline const char *c3_asan_default_options() {
    return "verbosity=0:debug=false"
           // Disabling leak detection as module isn't destoryed on exit
           ":detect_leaks=false"
           ":check_initialization_order=true"
           ":detect_stack_use_after_return=true";
}

#if defined(__cplusplus)
}
#endif  // defined(__cplusplus)
#endif  // MODULES_COMMON_CCSIMICS_SIMICS_UTIL_H_
