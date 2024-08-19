// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef C3LIB_C3_GENERIC_COREDUMPS_H_
#define C3LIB_C3_GENERIC_COREDUMPS_H_

#include "c3/generic/c3_conf.h"
#include "c3/generic/defines.h"

typedef struct __attribute__((__packed__)) cc_core_info {
    struct cc_context cc_context;
    size_t stack_rlimit_cur;
} cc_core_info_t;

#endif  // C3LIB_C3_GENERIC_COREDUMPS_H_
