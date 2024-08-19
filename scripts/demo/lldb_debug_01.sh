#!/usr/bin/env bash
# Copyright 2024 Intel Corporation
# SPDX-License-Identifier: MIT

set -euo pipefail

CMDLINE_ARGS=( "${@}" ); readonly CMDLINE_ARGS
SCRIPT_PATH=$(readlink -f -- "${BASH_SOURCE[0]}")
SCRIPT_NAME=$(basename -- "${BASH_SOURCE[0]}")
SCRIPT_DIR=$(cd -- "$(dirname -- "${SCRIPT_PATH}")" &> /dev/null && pwd)
readonly SCRIPT_PATH; readonly SCRIPT_DIR; readonly SCRIPT_NAME;

PROJECT_DIR=$(cd -- "${SCRIPT_DIR}/../.." &> /dev/null && pwd)

parse_cmdline_args() {
    local _bad_args=0

    if [ "${VERBOSE:=0}" -eq 1 ]; then
        verbose=1
    fi

    while [[ $# -gt 0 ]]; do
        opt="$1"

        case $opt in
        --help)
            echo
            echo "${SCRIPT_NAME}: No help :|"
            echo
            echo "${SCRIPT_NAME} at ${SCRIPT_DIR}"
            echo
            exit 0 ;;
        --skip-lldb)
            skip_lldb=1
            shift ;;
        --skip-run)
            skip_run=1
            shift ;;
        --verbose)
            verbose=1
            shift ;;
        *)
            echo "${SCRIPT_NAME}: Unrecognized argument: $1"
            (( _bad_args = _bad_args + 1 ))
            shift ;;
        esac
    done

    (( _bad_args == 0 )) && return 0
    exit 1
}

echo_info() {
    if [[ $verbose == 1 ]]; then
        echo "$@"
    fi
}

press_any_key_to_continue() {
    if [ -t 0 ]; then
        echo "press enter to continue"
        read -r
    fi
    return 0
}

parse_cmdline_args "${CMDLINE_ARGS[@]}"

readonly verbose=${verbose:=0}
readonly skip_run=${skip_run:=0}
readonly skip_lldb=${skip_lldb:=0}


readonly lldb_bin=${lldb_bin:="$(pwd)/llvm/llvm_install/bin/lldb"}

readonly runlog="${PROJECT_DIR}/debug/lldb_debug_01.log"
readonly debug_dir="${PROJECT_DIR}/debug"
readonly coredump_short_path="./debug/core"
readonly coredump_path="${PROJECT_DIR}/debug/core"
readonly lldb_script_peek1="${SCRIPT_DIR}/lldb_debug_01_peek1.lldb_script"
readonly lldb_script_peek2="${SCRIPT_DIR}/lldb_debug_01_peek2.lldb_script"
readonly lldb_script_base="${SCRIPT_DIR}/lldb_debug_01.lldb_script"

readonly lldb_script=${lldb_script:="${PROJECT_DIR}/debug/lldb_debug_01.lldb_script.mod"}

run_app_in_simics() {
    local awk_prompt='<board.serconsole.con>simics@board:~\$'
    local awk_runline='CC_ENABLED=1.*.\/a.out'
    local awk_retval_echo='simics@board:~\$ echo \$\?'

    local simics_args=(
        -batch-mode
        'scripts/runworkload_common.simics'
        'src_file=microbenchmarks/lldb_hello.cpp'
        'model=c3'
        'checkpoint=checkpoints/cc_kernel.ckpt'
        'unwinder=llvm_libunwind'
        'gcc_flags="-O2 -ldl -lm -lpthread -pthread -g -gdwarf -Werror -Iinclude"'
        'enable_coredumps=TRUE'
        'enable_cc_castack=TRUE'
    )

    local awk_start="${awk_prompt}.*${awk_runline}"
    local awk_end="${awk_retval_echo}"

    local filter=( "awk" "/${awk_start}/{flag=1;next}/${awk_end}/{flag=0}flag" )
    local cleanup=( sed 's/\\r\\n$//' )

    cat <<EOF
===
=== Running example application
===

    This will launch a new Simics instance, and run a "vulnerable" application
    that causes a buffer overflow. The appliication itself simulates an attack
    by modifying a local variable in an inline assembly snippet to cause" a for
    loop to overflow.

    The resulting core dump is then downloaded from the simulation to the host
    system for debugging in the following steps.

    The output is truncated, use can use --verbose to see all output.

EOF

    if [[ $verbose == 1 ]]; then
        filter=( "cat" )
        echo "Running: " ./simics "${simics_args[@]}" '2>&1' "|" "${filter[@]}" "|" "${cleanup[@]}"
    fi

    ./simics "${simics_args[@]}" 2>&1 | "${filter[@]}" | "${cleanup[@]}"

    if [[ ! -e "${coredump_path}" ]]; then
        echo
        echo "Cannot find coredump  at ${coredump_path}"
        echo
        echo "Pleasre re-run with --verbose to troubleshoot"
        echo
    fi

cat <<EOF

===
=== Done running Simics.
===
    coredump downloaded to $coredump_path"

EOF
    press_any_key_to_continue
}

check_lldb_exists() {
    if [[ ! -e "${lldb_bin}" ]]; then
        cat <<EOF

!!! Cannot find LLDB at ${lldb_bin}

Did you build it?

To build LLDB, run:
    make make_llvm-lldb-only
or, via docker
    make c3_docker-make_llvm-lldb-only

EOF
        exit 1
    fi
    return 0
}

peek_coredump_and_prep_script() {
    pushd "${debug_dir}" > /dev/null
    output=$("${lldb_bin}" -S "${lldb_script_peek1}")
    popd > /dev/null

    rsp=$(echo "$output" | grep -E '^\s*rsp\s*=\s*' | sed 's/.*=\s*//')
    end=$(echo "$output" \
          | grep -E '^\(int\s+\*\)\s+\$0\s*=\s*' \
          | awk '{print $5}')
    original_str=$(echo "$output" \
                   | grep -E '^\(char\s+\*const\)\s+\$1\s*=\s*' \
                   | awk '{print $5}')
    str=$(echo "$output" \
         | grep -E '^\(char\s+\*\)\s+\$2\s*=\s*' \
         | awk '{print $5}')

    pushd "${debug_dir}" > /dev/null
    output=$(sed "s/#rsp#/${rsp}/g" "${lldb_script_peek2}" | "${lldb_bin}")
    popd > /dev/null

    rsp_la=$(echo "$output" | grep -E '^decode_ptr:' | sed 's/.*->\s*//')

    if [[ $verbose == 1 ]]; then
        echo "Using following addresses from coredupmp to prep script:"
        echo "    rsp          -> ${rsp}"
        echo "    rsp_la       -> ${rsp_la}"
        echo "    end          -> ${end}"
        echo "    str          -> ${str}"
        echo "    original_str -> ${original_str}"
        echo
    fi

    cat "${lldb_script_base}" \
        | sed "s/#rsp#/${rsp}/g" \
        | sed "s/#rsp_la#/${rsp_la}/g" \
        | sed "s/#end#/${str}/g" \
        | sed "s/#original_str#/${original_str}/g" \
        | sed "s/#str#/${str}/g" \
        > "${lldb_script}"
}

run_lldb() {
    cat <<EOF
===
=== Starting LLDB and inspecting coredump (at debug/core)"
===

    Load C3-enchaned LLDB with prepared script to inspect the generated coredump
    file at ${coredump_short_path}.  The script will inspect the stack, the str
    buffer that caused the overflow, and local variables that may have caued the
    issue.

EOF
    press_any_key_to_continue

    check_lldb_exists
    peek_coredump_and_prep_script

    pushd "${debug_dir}" > /dev/null
    echo "${lldb_bin}" -S "${lldb_script}"
    # "${lldb_bin}" -S "${lldb_script}" | sed 's/^.lldb.\s*#\s*(.*)/\e[32mRed\$1\e[0m/'
    "${lldb_bin}" -S "${lldb_script}" | sed 's/^.lldb.\s*#\s*//'
    popd > /dev/null
}

[[ $skip_run == 1 ]] || run_app_in_simics
[[ $skip_lldb == 1 ]] || run_lldb
