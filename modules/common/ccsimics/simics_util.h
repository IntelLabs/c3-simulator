/*
 Copyright 2016 Intel Corporation
 SPDX-License-Identifier: MIT
*/
#ifndef MODULES_COMMON_CCSIMICS_SIMICS_UTIL_H_
#define MODULES_COMMON_CCSIMICS_SIMICS_UTIL_H_

#include <execinfo.h>
#include <iomanip>
#include <sstream>
#include <string>
#include <simics/arch/x86.h>
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
        ss << std::setw(2) << std::setfill('0') << std::hex
           << static_cast<int>(buf[i]);
    }
    return ss.str();
}

static inline const char *convert_reg_to_string(int reg) {
    switch (reg) {
    case X86_Reg_Id_PC:
        return "X86_Reg_Id_PC";
    case X86_Reg_Id_Rax:
        return "X86_Reg_Id_Rax";
    case X86_Reg_Id_Rbx:
        return "X86_Reg_Id_Rbx";
    case X86_Reg_Id_Rcx:
        return "X86_Reg_Id_Rcx";
    case X86_Reg_Id_Rdx:
        return "X86_Reg_Id_Rdx";
    case X86_Reg_Id_Rsp:
        return "X86_Reg_Id_Rsp";
    case X86_Reg_Id_Rbp:
        return "X86_Reg_Id_Rbp";
    case X86_Reg_Id_Rsi:
        return "X86_Reg_Id_Rsi";
    case X86_Reg_Id_Rdi:
        return "X86_Reg_Id_Rdi";
    case X86_Reg_Id_R8:
        return "X86_Reg_Id_R8";
    case X86_Reg_Id_R9:
        return "X86_Reg_Id_R9";
    case X86_Reg_Id_R10:
        return "X86_Reg_Id_R10";
    case X86_Reg_Id_R11:
        return "X86_Reg_Id_R11";
    case X86_Reg_Id_R12:
        return "X86_Reg_Id_R12";
    case X86_Reg_Id_R13:
        return "X86_Reg_Id_R13";
    case X86_Reg_Id_R14:
        return "X86_Reg_Id_R14";
    case X86_Reg_Id_R15:
        return "X86_Reg_Id_R15";
    default:
        return "X86_Reg_Id_Not_Used";
    }
}

static inline void print_simics_stacktrace() {
    constexpr const int arr_max_size = 1024;
    void *array[arr_max_size];

    auto size = backtrace(array, arr_max_size);
    auto strings = backtrace_symbols(array, size);

    if (strings != nullptr) {
        dbgprint("Stacktrace:");
        for (int i = 0; i < size; ++i) {
            SIM_printf("%s\n", strings[i]);
        }
    } else {
        dbgprint("Stacktrace failed");
    }

    free(strings);
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
