#!/bin/bash
# Copyright 2023 Intel Corporation
# SPDX-License-Identifier: MIT
# set -x
set -e

# SCRIPT_DEBUG=1
SCRIPT_DEBUG=${SCRIPT_DEBUG:=0}
debug=${debug:=1}

SCRIPT_PATH_NOLINK=$(readlink -f -- "${BASH_SOURCE[0]}")
SCRIPT_DIR="$( cd -- "$( dirname -- "${SCRIPT_PATH_NOLINK}" )" &> /dev/null && pwd )"
EDK2_SRC_DIR="$( cd -- "${SCRIPT_DIR}"/.. &> /dev/null && pwd )"
PROJECT_DIR="$( cd -- "${SCRIPT_DIR}"/../.. &> /dev/null && pwd )"

source "${PROJECT_DIR}/scripts/common_bash_lib.sh"

cwd=`realpath $0 ` # Get the path to this file
cwd=`dirname ${cwd}`

COMPILER=${COMPILER:="GCC5"}
if [[ $debug != 0 ]]; then
    file_path="${EDK2_SRC_DIR}/Build/SimicsOpenBoardPkg/BoardX58Ich10/DEBUG_$COMPILER/X64"
else
    file_path="${EDK2_SRC_DIR}/Build/SimicsOpenBoardPkg/BoardX58Ich10/RELEASE_$COMPILER/X64"
fi

readonly def_edk2_module_config="${EDK2_SRC_DIR}/configs/edk2_module_config.sh"

# The header_file_decl that contains only declarations
readonly header_file_decl="${EDK2_SRC_DIR}/edk2/MdePkg/Include/Library/C3ConstantSections.h"
# The header_file_defs that has the definition for the offset tables
readonly header_file_defs="${EDK2_SRC_DIR}/edk2/MdeModulePkg/Core/Dxe/Image/ConstantSectionOffset.h"

main() {
    # Load enabled modules from $edk2_module_config (or $def_edk2_module_config)
    load_module_config

    # # Run GenExtraConstants, if it exists
    # readonly extra_script="${SCRIPT_DIR}/GenExtraConstants.sh"
    # if [[ -e "$extra_script" ]]; then
    #     pushd "$cwd"
    #     "$extra_script"
    #     popd
    # fi

    # Generate $header_file_defs
    #
    # Go through all the modules in all_modules, and generate the $header_file_defs
    # header that has info on of globals+constant address range offsets for each
    # module that is enabled.
    #
    # Address ranges are determined by using objdjump -t and then using the offset
    # and symbol data size to calculate the min / max addresses to form the range.
    #

    {
        echo "/*"
        echo " * NOTE: Generated file, will be overwritten on re-build!"
        echo " */"
        echo
        echo "#ifndef CONSTANTSECTIONOFFSET_H"
        echo "#define CONSTANTSECTIONOFFSET_H"
        echo "#define NUMBER_OF_MODULES $num_modules"
        echo
        echo "/*"
        echo "All modules (enabled modules: ${num_modules}/${num_all_modules})"
        echo
        echo "${all_modules[@]}" | tr ' ' '\n'
        echo "*/"
        echo
        echo "const CHAR8* ModuleNames[NUMBER_OF_MODULES]= { \"TestDriver.efi\""
        for module in "${module_names[@]}"; do
            echo -e "\t\t\t, \"${module}.efi\""
        done
        echo "};"
        echo
        echo "const UINT32 ConstantOffsets[NUMBER_OF_MODULES] = {0x1"
    } > "${header_file_defs}"

    for module in "${module_names[@]}"; do
        local module_debug_path="$(get_module_debug_path "$module")"
        print_data_start_address "${module_debug_path}" "$header_file_defs"
    done

    {
        echo "};"
        echo ""
        echo "const UINT32 ConstantEndOffsets[NUMBER_OF_MODULES] = {0x1"
    } >> "${header_file_defs}"

    for module in "${module_names[@]}"; do
        local module_debug_path="$(get_module_debug_path "$module")"
        print_data_end_address "${module_debug_path}" "$header_file_defs"
    done

    {
        echo "};"
        echo
        echo "#endif"
    } >> "${header_file_defs}"

    {
        echo "/*"
        echo " * NOTE: Generated file, will be overwritten on re-build!"
        echo " */"
        echo
        echo "#ifndef CONSTANTSECTIONOFFSET_H"
        echo "#ifndef LIBRARY_C3_CONSTANT_SECTIONS_H_"
        echo "#define LIBRARY_C3_CONSTANT_SECTIONS_H_"
        echo
        echo "#define NUMBER_OF_MODULES $num_modules"
        echo
        echo "extern const CHAR8* ModuleNames[NUMBER_OF_MODULES];"
        echo "extern const UINT32 ConstantOffsets[NUMBER_OF_MODULES];"
        echo "extern const UINT32 ConstantEndOffsets[NUMBER_OF_MODULES];"
        echo
        echo "#endif"
        echo "#endif"
    } > "${header_file_decl}"
}

get_module_debug_path() {
    local module="$1"

    echo "$file_path/$module.debug"
}

# Loads either default module config
load_module_config() {

    #
    # Load the module list from config file. The config should define:
    #       module_names      <- A list of all modules to enable
    #       disabled_modules  <- A list of modules to disable
    # The enabled modules will then be ( module_names - disabled_modules ).
    #
    local module_config=${edk2_module_config:="${def_edk2_module_config}"}
    echo "=== Loading $module_config"

    [[ -e ${module_config} ]] || __die "Cannot find ${module_config}"
    # shellcheck source=../configs/edk2_module_config.sh
    source "${module_config}"

    # Remove duplicates from both lists
    module_names=($(echo "${module_names[@]}" | tr ' ' '\n' | sort -u | tr '\n' ' '))
    disabled_modules=($(echo "${disabled_modules[@]}" | tr ' ' '\n' | sort -u | tr '\n' ' '))

    for d in "${disabled_modules[@]}"; do
        for i in "${!module_names[@]}"; do
            if [[ "${module_names[i]}" == "${d}" ]]; then
                unset 'module_names[i]'
            fi
        done
    done

    # Remove empty strings
    local tmp_module_names=( "${module_names[@]}" )
    module_names=()
    for m in "${tmp_module_names[@]}"; do
        if [[ ! $m =~ ^\s*$ ]]; then
            module_names+=( "$m" )
        fi
    done

    # Calculate the number of enabled modules
    num_modules=$(( 1 + "${#module_names[@]}" ))

    # Retrieve the number of modules by inspecting actual object files
    all_modules=($(find "$file_path" -name '*.efi' -exec basename '{}' \; | sort -u | tr '\n' ' '))
    num_all_modules=$(( 1 + "${#all_modules[@]}" ))

    readonly module_names
    readonly all_modules
    readonly num_modules
    readonly num_all_modules

    # echo "${module_names[@]}"
    #     for module in "${module_names[@]}"; do
    #         echo -e "\t\t\t, \"${module}.efi\""
    #     done
    # exit
}

find_object_size() {
    local res="$1"
    local fn="$2"
    local sym="$3"

    printf -v "$res" "%s" "$(nm --print-size --size-sort "$fn" | \
                             grep "$sym" | awk -e '{print $2}')"
}

hex_add() {
    local A="$1"
    local B="$2"
    printf "%x" "$(( 16#$A + 16#$B ))"
}

hex_sub() {
    local A="$1"
    local B="$2"
    printf "%x" "$(( 16#$A - 16#$B ))"
}

cmp_pick_smallest() {
    local A="$1"
    local B="$2"
    if [[ -z "$A" ]]; then
        echo "$B"
    elif [[ -z "$B" ]]; then
        echo "$A"
    else
        printf "%x" "$(( 16#$A > 16#$B ? 16#$B : 16#$A))"
    fi
}

cmp_pick_largest() {
    local A="$1"
    local B="$2"
    if [[ -z "$A" ]]; then
        echo "$B"
    elif [[ -z "$B" ]]; then
        echo "$A"
    else
        printf "%x" "$(( 16#$A < 16#$B ? 16#$B : 16#$A))"
    fi
}

find_objdump_label() {
    __find_objdump_label "$1" "$2" "$3" "$4" 0
}

find_objdump_label_rev() {
    __find_objdump_label "$1" "$2" "$3" "$4" 1
}

__find_objdump_label() {
    local res="$1"
    local fn="$2"
    local regexp="$3"
    local size_field="$4"
    local find_end="$5"

    local objdump_args=(objdump -t "$fn")
    local grep_args=(grep -P "${regexp}")
    local sort_args=(sort)
    local head_args=(head -1)
    local sed_args=( sed -E 's/\s*//g' )
    local awk_args=(awk "{print \$1 \" \" \$${size_field}}")
    local size_offset=0
    local address=0
    local sym=""

    if [[ $find_end != 0 ]]; then
        sort_args+=( -r )
    fi

    address="$("${objdump_args[@]}" | \
               "${grep_args[@]}" | \
               "${sort_args[@]}" | \
               "${head_args[@]}" | \
               "${awk_args[@]}")"

    if [[ -z $address ]]; then
        __debug_say "Did not find $regexp in $fn"
    else
        __debug_say "Found $regexp at $address"
        if [[ $find_end != 0 ]]; then
            sym=$( echo "$address" | awk '{print $2}' )
            size_offset="$( echo "$address" | awk '{print $2}' )"
        fi
        address="$( echo "$address" | awk '{print $1}' )"
        address="$( hex_add "$address" "$size_offset" )"
    fi

    printf -v "$res" "%x" "$(( 16#$address ))"
}

print_data_start_address() {
    local fn="$1"
    local fn_out="$2"
    local address_lc0=""
    local address_data=""

    __debug_say "__print_data_start ${fn}"

    if [[ -e "$fn" ]]; then
        __debug_say "Found file ${fn}"
        find_objdump_label address_lc0 "${fn}" "LC0" 4
        find_objdump_label address_data "${fn}" '\s.data\s' 5
        __debug_say "lc0:           $address_lc0"
        __debug_say "data_start:    $address_data"
        address_first="$(cmp_pick_smallest "$address_lc0" "$address_data")"

        if [[ $fn == */BdsDxe.debug ]]; then
            # Ugly hack to ensure we encrypt switch metadata at end of .text
            # section that is going to be accessed with GSRIP:ed CA.
            __debug_say "Adjusting address_first from $address_first"
            address_first=$(hex_sub "$address_first" 14)
            __debug_say "                          to $address_first"
        fi

        if [[ $fn == */DisplayEngine.debug ]]; then
            # Ugly hack to ensure we encrypt switch metadata at end of .text
            # section that is going to be accessed with GSRIP:ed CA.
            __debug_say "Adjusting address_first from $address_first"
            address_first=$(hex_sub "$address_first" 1c)
            __debug_say "                          to $address_first"
        fi

        __debug_say "address_first: $address_first"

        if [[ -z "$address_first" ]]; then
            echo -e "\n"                                                    \
                    "!!! -------------------------------------------\n"     \
                    "!!! Cannot find data/constant start in\n"              \
                    "!!! \t$fn\n"                                           \
                    "!!!"                                                   \
                    "!!! lc0:                $address_lc0\n"                \
                    "!!! address_data_start: $address_data\n"               \
                    "!!! -------------------------------------------\n"
        fi

        echo -e "\t\t\t, 0x$address_first" >> "$fn_out"
    else
        echo -e "\t\t\t, 0x0" >> "$fn_out"
    fi
}

print_data_end_address() {
    local fn="$1"
    local fn_out="$2"
    local address_lc0=""
    local address_data=""

    if [[ -e "$fn" ]]; then
        find_objdump_label_rev address_lc0 "$1" "LC0" 4
        find_objdump_label_rev address_data "$1" '\s.data\s' 5
        address_first="$(cmp_pick_largest "$address_lc0" "$address_data")"

        if [[ -z "$address_first" ]]; then
            echo "!!! Cannot find data/constant end in $fn"
        fi
        echo -e "\t\t\t, 0x$address_first" >> "$fn_out"
    else
        echo -e "\t\t\t, 0x0" >> "$fn_out"
    fi
}

main "$@"
