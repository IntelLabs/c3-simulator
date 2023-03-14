/*
 Copyright 2023 Intel Corporation
 SPDX-License-Identifier: MIT
*/
#pragma once
#include <stdint.h>

uint64_t ascon64b_stream(uint64_t data_in, const uint8_t *key);
