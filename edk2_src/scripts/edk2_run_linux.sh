#!/usr/bin/env bash
#
# Copyright 2024 Intel Corporation
# SPDX-License-Identifier: MIT

set -euo pipefail

# SCRIPT_DEBUG=1

SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
SCRIPT_NAME="$(basename -- "${BASH_SOURCE[0]}")"
C3_ROOT="$( cd -- "${SCRIPT_DIR}/../.." &> /dev/null && pwd )"
EDK2_SRC="$( cd -- "${C3_ROOT}/edk2_src" &> /dev/null && pwd )"
readonly SCRIPT_DIR
readonly C3_ROOT
readonly EDK2_SRC

# shellcheck source=../../scripts/run_buildroot_common.sh
. "${C3_ROOT}/scripts/run_buildroot_common.sh"
# shellcheck source=../../scripts/common_bash_lib.sh
. "${C3_ROOT}/scripts/common_bash_lib.sh"

__debug_say "Using SCRIPT_DIR: ${SCRIPT_DIR}"
__debug_say "Using C3_ROOT:    ${C3_ROOT}"
__debug_say "Using EDK2_SRC:   ${EDK2_SRC}"

set_variables() {
    local l_conf="${disable_ptrenc:=0}${disable_data_encryption:=0}${enable_integrity:=0}"
    local l_time
    l_time="$(date -Iseconds)"

    export simics_script="${SCRIPT_DIR}/edk2_run_linux.simics"
    export buildroot_dest="${EDK2_SRC}/edk2_buildroot"

    buildroot_config_file="${C3_ROOT}/scripts/buildroot_config/edk2_buildroot.config"
    # If buildroot_toolchain exists, then we will use alternative config
    readonly buildroot_toolchain="/opt/simics/buildroot_toolchains/x86_64-buildroot-linux-gnu_sdk-buildroot.tar.gz"
    if [[ -e "${buildroot_toolchain}" ]]; then
        buildroot_config_file="${C3_ROOT}/scripts/buildroot_config/edk2_buildroot_external_toolchain.config"
    fi
    export buildroot_config_file

    export buildroot_linux_config="${C3_ROOT}/linux/configs/linux_for_edk2_buildroot.config"
    export logfile="${C3_ROOT}/logs/edk2_run_linux.${l_time}.${l_conf}"
    __run_or_dry mkdir -p "$(dirname "${logfile}")"

    export simics_build_args+=( CC_EDK2=1 )
    __debug_say "set_variables: Set ${SCRIPT_NAME} variables"
}

link_latest_logs() {
    local logfile_path="$1"
    local log_basename
    local log_dirname
    log_basename="$(basename ${logfile})"
    log_dirname="$(dirname "${logfile_path}")"

    pushd "${log_dirname}" > /dev/null
    __run_or_dry ln -f -s "${log_basename}.edk2.commandline.txt" edk2_run_linux.latest.edk2.commandline.txt
    __run_or_dry ln -f -s "${log_basename}.commandline.txt" edk2_run_linux.latest.commandline.txt
    __run_or_dry ln -f -s "${log_basename}.serconsole.txt" edk2_run_linux.latest.serconsole.txt
    popd
}

main() {
    # Parse args and set defaults
    parse_args "$@"

    do_common_options

    [[ ${rebuild_edk2:=0} == 0 ]] || do_rebuild_edk2

    if [[ $myfunc == "__do_run" ]]; then
        # Link logfile for convenience
        link_latest_logs "${logfile}"
    fi

    __debug_say "main: Running \$myfunc (which is ${myfunc:=""})"
    [[ -z ${myfunc:=""} ]] || $myfunc

    return 0
}

# Used to set fixed buildroot checksum for debugging scripts
# buildroot_config_checksum() {
#     local varname="$1"
#     read -r "${varname?}" <<< "30ad545feb33edad"
# }

parse_extra_args() {
    POSITIONAL=()
    while [[ $# -gt 0 ]]; do
        key="$1"

        case $key in # command-line options
            parse_log_globals)
                readonly myfunc=do_parse_log_globals
                globals_log="$2";
                shift; shift
                if [[ "$#" -gt 1 ]] && [[ "$1" == "search" ]]; then
                    globals_log_cmd="search"
                    globals_log_cmd_arg="$2"
                    shift; shift;
                fi
                ;;
            --break_on_integrity_fault)
                simics_args+=( break_on_integrity_fault=1 )
                shift;;
            --rebuild_edk2)
                rebuild_edk2=1
                shift ;;
            *)
            POSITIONAL+=("$1") # save it in an array for later
            shift # past argument
            ;;
        esac
    done

    if [[ -n "${POSITIONAL[*]}" ]]; then
        __die "Unknown args: " "${POSITIONAL[@]}"
    fi
}

__globals_log_name() {
    echo "$1" | awk '{print $3}' | sed 's/.efi,$//'
}

__globals_log_start() {
    __hex "$(echo "$1" | awk '{print $5}' | sed 's/,$//')"
}

__globals_log_end() {
    __hex "$(echo "$1" | awk '{print $7}' | sed 's/,$//')"
}

__globals_log_gstart() {
    __hex "$(echo "$1" | awk '{print $9}' | sed 's/,$//')"
}

__globals_log_static_offset() {
    __hex_sub "$(__hex "$2")" "$(__globals_log_start "$1")"
}

__globals_log_gend() {
    local line="$1"
    local start
    local size
    start=$(echo "$line" | awk '{print $9}' | sed 's/,$//' | sed 's/^0*//')
    size=$(echo "$line" | awk '{print $11}' | sed 's/,$//' | sed 's/^0*//')
    __hex_add "${start}" "${size}"
}

do_parse_log_globals() {
    local fn=${globals_log:=""}
    [[ -z "${fn}" ]] && __die "Need path to Simics log file"

    local count_e
    local count_d
    local count_t
    local enabled
    local disabled
    local all_modules

    readarray -t all_modules < <(grep -E '\[C3_GLOBALS\]: (SMM-)?(Enable|Disable)' "$fn")
    # TODO: Just do one grep of the file

    readarray -t enabled < <(grep '\[C3_GLOBALS\]: Enabled' "$fn" \
        | awk '{print $3}' | sed 's/.efi,$//' | sort)

    readarray -t disabled < <(grep '\[C3_GLOBALS\]: Disabled' "$fn" \
        | awk '{print $3}' | sed 's/.efi,$//' | sort)

    count_e=$(echo "${enabled[*]}" | wc | awk '{print $2}')
    count_d=$(echo "${disabled[*]}" | wc | awk '{print $2}')
    count_t=$(( count_e + count_d ))

    readarray -t smm_enabled < <(grep '\[C3_GLOBALS\]: SMM-Enable' "$fn" \
        | awk '{print $3}' | sed 's/.efi,$//' | sort | sed 's/^SMM-//')

    readarray -t smm_disabled < <(grep '\[C3_GLOBALS\]: SMM-Disable' "$fn" \
        | awk '{print $3}' | sed 's/.efi,$//' | sort | sed 's/^SMM-//')

    smm_count_e=$(echo "${smm_enabled[*]}" | wc | awk '{print $2}')
    smm_count_d=$(echo "${smm_disabled[*]}" | wc | awk '{print $2}')
    smm_count_t=$(( smm_count_e + smm_count_d ))

    tot_count_e=$(( count_e + smm_count_e ))
    tot_count_t=$(( count_t + smm_count_t ))

    cat <<EOF
[C3_GLOBALS]: Enabled ${count_e} modules: ${enabled[@]}
[C3_GLOBALS]: Disabled ${count_d} modules: ${disabled[@]}
[C3_GLOBALS]: === Modules enabled at run-time: ${count_e}/${count_t}
[C3_GLOBALS]: Enabled ${smm_count_e} SMM modules: ${smm_enabled[@]}
[C3_GLOBALS]: Disabled ${smm_count_d} SMM modules: ${smm_disabled[@]}
[C3_GLOBALS]: === SMM modules enabled at run-time: ${smm_count_e}/${smm_count_t}
[C3_GLOBALS]: === Total modules enabled at run-time: ${tot_count_e}/${tot_count_t}
EOF

    if [[ ${globals_log_cmd:=""} == "search" ]]; then
        local target_plain="${globals_log_cmd_arg}"
        local target="0x${globals_log_cmd_arg}"
        local found_module=""

        __debug_say "Searching for address ${target}"

        for l in "${all_modules[@]}"; do
            local start; start="0x$(__globals_log_start "$l")"
            local end; end="0x$(__globals_log_end "$l")"
            local module_name; module_name=$(__globals_log_name "$l")

            __debug_say "Checking ${module_name} with range [${start}, ${end})"

            if (( start <= target && target < end)); then
                local gstart; gstart="0x$(__globals_log_gstart "$l")"
                local gend; gend="0x$(__globals_log_gend "$l")"

                [[ -z "${found_module}" ]] || __die "Multiple results found!"
                found_module="${module_name}"

                echo
                echo "Found $target in $module_name [$start, $end)"
                echo "    Module full range is [$start, $end)"
                echo -n "    Module vars range is [$gstart, $gend)"
                if (( gstart <= target && target < gend)); then
                    echo " ($target in variable range)"
                else
                    echo " (not in global range)"
                fi

                echo "    static offset: $(__globals_log_static_offset "${l}" "${target_plain}")"
            fi
        done

        if [[ -z "${found_module}" ]]; then
            echo
            echo "Cannot find ${target}"
        fi
    fi
}

main "$@"
