#!/usr/bin/env bash
#
# Copyright 2024 Intel Corporation
# SPDX-License-Identifier: MIT


if [ ! -v C3_ROOT ]; then
    RUN_BUILDROOT_COMMON_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
    C3_ROOT="$( cd -- "${RUN_BUILDROOT_COMMON_DIR}/../.." &> /dev/null && pwd )"
    readonly C3_ROOT
fi

# shellcheck source=common_bash_lib.sh
. "${C3_ROOT}/scripts/common_bash_lib.sh"

readonly _def_logfile="${C3_ROOT}/logs/run_buildroot_common.${disable_ptrenc:=0}${disable_data_encryption:=0}"
readonly _def_buildroot_user_tables="${C3_ROOT}/scripts/buildroot_config/users.table"
readonly _def_buildroot_linux_config="${C3_ROOT}/linux/configs/buildroot_default.config"
readonly _def_simics_script="${C3_ROOT}/run_buildroot.simics"
readonly _def_buildroot_config_file="${C3_ROOT}/scripts/buildroot_config/buildroot.c3.config"
readonly _def_buildroot_git_url="https://gitlab.com/buildroot.org/buildroot.git"
readonly _def_buildroot_git_rev="2024.02"
readonly _def_buildroot_dst="${C3_ROOT}/buildroot"

__usage() {
    if command -v usage > /dev/null 2>&1; then
        usage
    fi
}

do_common_options() {
    [[ ${rebuild_c3:=0} == 0 ]] || __do_rebuild_c3
}

# parse_arges "$@"
#
# This parses command line arguments passed in as function parameters. If
# defined, this will call parse_extra_args with any arguments not recognized
# by this common function.
#
# Finally, this function also locks down various variables, if not defined,
# with default values. To allow overrides, you can define a set_variables
# function that will be called before this.
#
# Common usage would be something like:
#
#   main() {
#       prase_args "$@"
#       do_common_options
#        ... do your thing here ...
#
#       # Run command, assuming it was set
#       [[ -z ${myfunc:=""} ]] || $myfunc
#
#       return 0
#   }
#
#   set_variables() {
#       # ... set your own default variables here ...
#   }
#
#   parse_extra_args() {
#       # ... handle scrpt-sepcific arguments here ...
#   }
#
#   main "$@"
#
parse_args() {
    # Options to pass to make when building the Simics model
    simics_build_args=()
    # Options to pass to Simics when running the model
    simics_args=()

    POSITIONAL=()
    while [[ $# -gt 0 ]]; do
        key="$1"

        case $key in # command-line options
            build)
                readonly myfunc=__do_build
                shift ;;
            run)
                readonly myfunc=__do_run
                shift ;;
            setup)
                readonly myfunc=__do_setup
                shift ;;
            source)
                readonly myfunc=__do_source
                shift ;;
            clone_buildroot)
                readonly myfunc=__do_clone_buildroot
                shift ;;
            buildroot-config)
                readonly myfunc=__do_buildroot_config
                shift ;;
            linux-config)
                readonly myfunc=__do_linux_config
                shift ;;
            -n)
                DRY_RUN=1
                shift ;;
            --batch-mode)
                simics_args+=( --batch-mode )
                shift;;
            --disable_c3)
                enable_c3=0
                shift;;
            --disable_ptrenc)
                disable_ptrenc=1
                shift ;;
            --disable_data_encryption)
                disable_data_encryption=1
                shift ;;
            --break_on_exception)
                break_on_exception=1
                shift ;;
            --enable_integrity)
                enable_integrity=1
                shift ;;
            --debug_on)
                debug_on=1
                shift ;;
            --magic)
                magic=1
                shift ;;
            --do_quit)
                do_quit=1
                shift ;;
            --rebuild_c3)
                rebuild_c3=1
                shift ;;
            --gdb_port)
                gdb_port="$2"
                shift ; shift ;;
            --net)
                connect_real_network=1
                shift ;;
            -h|--help)
                __usage
                exit ;;
            *)  # unknown option
            POSITIONAL+=("$1") # save it in an array for later
            shift # past argument
            ;;
        esac
    done

    # Call parse_extra_args if defined
    if [[ -n "${POSITIONAL[*]}" ]]; then
        __debug_say "parse_args: Trying parse_extra_args for unknown args"
        if command -v parse_extra_args > /dev/null 2>&1; then
            parse_extra_args "${POSITIONAL[@]}"
        else
            __die "Unknown args: " "${POSITIONAL[@]}"
        fi
    fi

    __debug_say "parse_args: Calling set_variables if defined"
    # Call set_variables if defined
    if command -v set_variables > /dev/null 2>&1; then
        set_variables
    fi

    simics_build_args+=( -C "${C3_ROOT}" -j"$(nproc)" -B )
    readonly simics_build_args

    readonly simics_args

    readonly logfile="${logfile:="${_def_logfile}"}"

    readonly simics_script=${simics_script:="${_def_simics_Script}"}

    readonly buildroot_git_url=${buildroot_git_url:="${_def_buildroot_git_url}"}
    readonly buildroot_git_rev=${buildroot_git_rev:="${_def_buildroot_git_rev}"}

    readonly buildroot_config_file=${buildroot_config_file:="${_def_buildroot_config_file}"}
    readonly buildroot_user_tables=${buildroot_user_tables:="${_def_buildroot_user_tables}"}
    readonly buildroot_linux_config=${buildroot_linux_config:="${_def_buildroot_linux_config}"}

    readonly buildroot_make_args=( -j"$(nproc)" )

    if [[ ! -v buildroot_rev ]]; then
        __debug_say "parse_args: Calling __buildroot_config_checksum"
        __buildroot_config_checksum buildroot_rev
    fi
    readonly buildroot_rev
    __debug_say "parse_args: buildroot_rev = ${buildroot_rev}"
    readonly buildroot_dest=${buildroot_dest:="${_def_buildroot_root}"}
    readonly buildroot_root="${buildroot_dest}/${buildroot_rev}"

    readonly buildroot_root_latest="${buildroot_dest}/latest"
    readonly buildroot_output_images="${buildroot_root_latest}/output/images"
    readonly kernel="${buildroot_output_images}/bzImage"
    readonly rootfs_image="${buildroot_output_images}/rootfs.ext2"

    __debug_say "parse_args: DONE"
}

__buildroot_config_checksum() {
    local varname="$1"
    local output

    # Define a buildroot_config_checksum function to override this if needed
    if command -v buildroot_config_checksum > /dev/null 2>&1; then
        buildroot_config_checksum "${varname}"
        return 0
    fi
    # Generate checksum of following files:
    local buildroot_config_files=(
        "${BASH_SOURCE[0]}"
        "${buildroot_user_tables}"
        "${buildroot_config_file}"
        "${buildroot_linux_config}"
    )

    # Include the following varibles in the final checksum:
    local buildroot_config_labels=(
        "${buildroot_git_rev}"
    )

    # Make sure all the files we try to use actually exist
    for f in "${buildroot_config_files[@]}"; do
        [ -e "$f" ] || __die "Cannot find $f"
    done

    output=$(sha256sum <<EOF | awk '{print $1}' | head -c16
$(sha256sum "${buildroot_config_files[@]}" | awk '{print $1}')
${buildroot_config_labels[*]}
EOF
)
    read -r "${varname?}" <<< "${output}"
}

__set_buildroot_conf() {
    local conf="$1"
    local var="$2"
    local val="$3"

    __run_or_dry sed -i "s|^${var}=.*$|${var}=\"${val}\"|" "${conf}"
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

__do_clone_buildroot() {
    local git_clone_args=( clone "$buildroot_git_url" -b "$buildroot_git_rev"
        --depth 1 "$buildroot_root" )
    [[ -e "$buildroot_root/.git" ]] || __run_or_dry git "${git_clone_args[@]}"
}

__do_setup() {
    local conf="${buildroot_root}/.config"

    # Remove old latest link
    rm -rf "${buildroot_root_latest}"

    __do_clone_buildroot
    # Run distclean if it looks like we have a repository here
    [[ ! -e "${buildroot_root}/.git" ]] || \
        __run_or_dry make -C "${buildroot_root}" distclean
    __run_or_dry cp "${buildroot_config_file}" "${conf}"
    __set_buildroot_conf "${conf}" "BR2_ROOTFS_USERS_TABLES" "${buildroot_user_tables}"
    __set_buildroot_conf "${conf}" "BR2_LINUX_KERNEL_CUSTOM_CONFIG_FILE" "${buildroot_linux_config}"
    __run_or_dry make -C "${buildroot_root}" "${buildroot_make_args[@]}" oldconfig

    # Re-link the latest to the buildroot we just set up
    pushd "$(dirname "${buildroot_root}")" || return
    ln -s "${buildroot_rev}" latest
    popd || return
}

__do_source() {
    __run_or_dry make -C "${buildroot_root}" "${buildroot_make_args[@]}" source
}

__do_buildroot_config() {
    __run_or_dry make -C "${buildroot_root}" "${buildroot_make_args[@]}" menuconfig
}

__do_linux_config() {
    __run_or_dry make -C "${buildroot_root}" "${buildroot_make_args[@]}" linux-menuconfig
}

__do_build() {
    __run_or_dry make -C "${buildroot_root}" "${buildroot_make_args[@]}" all
}

__do_rebuild_c3() {
    __run_or_dry make "${simics_build_args[@]}"
}

__do_run() {
    local kernel_cmdline=(
        console=tty0 "console=ttyS0,115200n8" earlyprintk root=/dev/vda ro
        # apic=verbose
        # debug_locks_verbose=1
        # hpet=verbose
        # rcuperf.verbose=1
        # sched_debug=1
        # nosplash
        # nokaslr
    )

    local all_simics_args=(
        "$simics_script"
        kernel="${kernel:=$kernel_buildroot}"
        enable_c3="${enable_c3:=1}"
        enable_integrity="${enable_integrity:=0}"
        disable_ptrenc="${disable_ptrenc:=0}"
        break_on_exception="${break_on_exception:=0}"
        disable_data_encryption="${disable_data_encryption:=0}"
        debug_on="${debug_on:=0}"
        do_quit="${do_quit:=0}"
        magic="${magic:=0}"
        logfile="${logfile}"
        gdb_port="${gdb_port:=0}"
        connect_real_network="${connect_real_network:=0}"
        virtio_rootfs="$rootfs_image"
        kernel_cmdline="${kernel_cmdline[*]}"
    )

    all_simics_args+=( "${simics_args[@]}" )

    [[ ! -v bios ]] || all_simics_args+=( bios="${bios}" )

    # Make sure the log directory exists
    __run_or_dry mkdir -p "$(dirname "$logfile")"

    # Then run simics
    pushd "$C3_ROOT" || return
    __run_or_dry ./simics "${all_simics_args[@]}"
    popd || return
}
