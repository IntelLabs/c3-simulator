// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef UNIT_TESTS_INCLUDE_UNIT_TESTS_ASSERTIONS_H_
#define UNIT_TESTS_INCLUDE_UNIT_TESTS_ASSERTIONS_H_

/**
 * @brief Check whether combination of pass-by-reference and pass-by-value
 * values are the same
 *
 * Use pass-by-reference (i.e., pointer's to values) and pass-by-value and try
 * to ensure compiler cannot be too clever and optimize away memory ops that may
 * be under test
 *
 * @tparam Ty
 */

template <typename Ty>
__attribute__((noinline)) static bool is_equal_val_val(Ty ptr1, Ty ptr2) {
    asm("mov %0, %%rcx\n"
        "mov %1, %%rcx\n"
        "nop\n"
        : "+a"(ptr1), "+b"(ptr2)
        :
        // Tell the compiler the inline-ASM may modify memory to ensure it
        // cannot make any safe assumptions about the values *ptr1 and *ptr2
        : "memory", "%rcx");
    return ptr1 == ptr2;
}

template <typename Ty>
__attribute__((noinline)) static bool is_equal_ptr_val(Ty *ptr1, Ty ptr2) {
    asm("mov %0, %%rcx\n"
        "mov %1, %%rcx\n"
        "nop\n"
        : "+a"(ptr1), "+b"(ptr2)
        :
        // Tell the compiler the inline-ASM may modify memory to ensure it
        // cannot make any safe assumptions about the values *ptr1 and *ptr2
        : "memory", "%rcx");
    return *ptr1 == ptr2;
}

template <typename Ty>
__attribute__((noinline)) static bool is_equal_ptr_ptr(Ty *ptr1, Ty *ptr2) {
    asm("mov %0, %%rcx\n"
        "mov %1, %%rcx\n"
        "nop\n"
        : "+a"(ptr1), "+b"(ptr2)
        :
        // Tell the compiler the inline-ASM may modify memory to ensure it
        // cannot make any safe assumptions about the values *ptr1 and *ptr2
        : "memory", "%rcx");
    return *ptr1 == *ptr2;
}

template <typename Ty>
__attribute__((noinline)) static bool is_different_val_val(Ty ptr1, Ty ptr2) {
    return !is_equal_val_val(ptr1, ptr2);
}
template <typename Ty>
__attribute__((noinline)) static bool is_different_ptr_val(Ty *ptr1, Ty ptr2) {
    return !is_equal_ptr_val(ptr1, ptr2);
}
template <typename Ty>
__attribute__((noinline)) static bool is_different_ptr_ptr(Ty *ptr1, Ty *ptr2) {
    return !is_equal_ptr_ptr(ptr1, ptr2);
}
#endif  // UNIT_TESTS_INCLUDE_UNIT_TESTS_ASSERTIONS_H_
