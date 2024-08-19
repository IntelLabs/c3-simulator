// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: MIT

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define _CC_GLOBALS_NO_INCLUDES_
#include "cc_globals.h"  // NOLINT

int main(void) {
    cc_trigger_icv_map_reset();
    fflush(stdin);
    return 0;
}
