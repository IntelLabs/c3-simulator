#ifndef UNIT_TESTS_INCLUDE_UNIT_TESTS_STACK_COMMON_H_
#define UNIT_TESTS_INCLUDE_UNIT_TESTS_STACK_COMMON_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// These are some static vars for debugging and confusing compiler
static uintptr_t __s_rsp;   // Temp variable to store %rsp
void *__s_leak_ptr;         // For leaking pointer to confuse compiler
size_t __s_buff_size = 16;  // Changeable size to confuse compiler

__attribute__((noinline)) static void leak_ptr(void *ptr) {
    __s_leak_ptr = ptr;
}

/**
 * @brief printf to stdout and do fflush(stdout)
 */
#define say(...)                                                               \
    do {                                                                       \
        printf(__VA_ARGS__);                                                   \
        fflush(stdout);                                                        \
    } while (false)

/**
 * @brief Indirection to print out __s_rsp value (to avoid clutter in caller)
 */
static __attribute((noinline)) void __dump_rsp() {
    say("rsp: 0x%016lx\n", __s_rsp);
}

/**
 * @brief Store %rsp value in _val
 */
#define get_sp(_val)                                                           \
    do {                                                                       \
        __asm("mov %%rsp, %[r];\n\t" : [ r ] "=r"(_val) : :);                  \
    } while (false)

/**
 * @brief Get current %rsp value and print it out
 */
#define dump_rsp()                                                             \
    do {                                                                       \
        get_sp(__s_rsp);                                                       \
        __dump_rsp();                                                          \
    } while (false)

/**
 * @brief Allocate and leak a stack buffer to force compiler to push %rbp
 */
#define force_rbp_store()                                                      \
    __s_leak_ptr = alloca(__s_buff_size < 64 ? ++__s_buff_size : __s_buff_size)

#endif  // UNIT_TESTS_INCLUDE_UNIT_TESTS_STACK_COMMON_H_
