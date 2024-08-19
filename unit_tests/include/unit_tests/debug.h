// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef UNIT_TESTS_INCLUDE_UNIT_TESTS_DEBUG_H_
#define UNIT_TESTS_INCLUDE_UNIT_TESTS_DEBUG_H_

/**
 * @brief select level of debug print
 *
 * Define different levels of print for debug based on MACROS defined per
 * purpose of executable E.g., RTL simulations do not allow print selection
 * macro are mutually exclusive or print have different name  to prevent
 * redefinition E.g., RTL_ENV or SIMICS_ENV, or UNIX
 *
 *
 */

// valprint
// avoid printf, kprint, dbgprint in RTL env
// customized printf in Unix or Simics
#ifdef RTL_ENVIRONMENT
#define valprint(f, ...)
#define kprintf fprintf
#else
#define valprint(f, ...)                                                       \
    printf("(%s:%d) %s: " f "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#endif  // C3_RTL_TEST_ENVIRONMENT

#endif  // UNIT_TESTS_INCLUDE_UNIT_TESTS_DEBUG_H_
