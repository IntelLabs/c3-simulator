#!/usr/bin/env bash
# Copyright 2024 Intel Corporation
# SPDX-License-Identifier: MIT

[[ ${_common_bash_lib_sh_:=0} == 1 ]] && return 0
_common_bash_lib_sh_=1

__run_or_dry() {
    [[ ${DRY_RUN:=0} == 0 ]] || echo -n "DRY_RUN: "
    echo "$@"
    [[ ${DRY_RUN:=0} != 0 ]] || "$@"
}

__die_val() {
    exit_value=$1
    shift
    echo "$@"
    exit "${exit_value}"
}

__die() {
    __die_val 1 "$@"
}

__debug_say() {
    [[ ${SCRIPT_DEBUG:=0} == 0 ]] || echo "$@"
}

__hex() {
    local A="$1"
    A=${A//^0*/}
    printf "%x" "$(( 16#$A  ))"
}

__hex_add() {
    local A="$1"
    local B="$2"
    printf "%x" "$(( 16#$A + 16#$B ))"
}

__hex_sub() {
    local A="$1"
    local B="$2"
    printf "%x" "$(( 16#$A - 16#$B ))"
}
