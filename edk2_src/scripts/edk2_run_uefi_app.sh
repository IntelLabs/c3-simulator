#! /usr/bin/env bash

set -euo pipefail

SCRIPT_PATH_NOLINK=$(readlink -f -- "${BASH_SOURCE[0]}")
SCRIPT_DIR="$( cd -- "$( dirname -- "${SCRIPT_PATH_NOLINK}" )" &> /dev/null && pwd )"
EDK2_SRC_DIR="$( cd -- "${SCRIPT_DIR}"/.. &> /dev/null && pwd )"
PROJECT_DIR="$( cd -- "${SCRIPT_DIR}"/../.. &> /dev/null && pwd )"

dumper() {
    echo "$1=${!1}"
}

dumper SCRIPT_PATH_NOLINK
dumper SCRIPT_DIR
dumper PROJECT_DIR

while [[ $# -gt 0 ]]; do
  key="$1"
  case $key in
    -uefi_app)
        uefi_app="$2"
        shift # past argument
        shift # past value
        ;;
    -app_debug)
        app_debug=1
        shift; ;;
    -build_edk2)
        build_edk2=1
        shift; ;;
    -build_c3)
        build_c3=1
        shift; ;;
    -disable_ptrenc)
        disable_ptrenc=1
        shift; ;;
    -enable_integrity)
        enable_integrity=1
        shift; ;;
    -disable_data_encryption)
        disable_data_encryption=1
        shift; ;;
    -allow_scan)
        allow_scan=1
        shift; ;;
    -magic)
        magic=1
        shift ;;
    *)    # unknown option
        echo "Unknown $key, quitting"
        exit 1
        ;;
  esac
done

export debug=${debug:=1}
export COMPILER=${COMPILER:="GCC5"}
build_path="${EDK2_SRC_DIR}/Build/SimicsOpenBoardPkg/BoardX58Ich10"
if [[ ${debug:=1} != 0 ]]; then
    build_type="DEBUG"
else
    build_type="RELEASE"
fi
readonly build_path="${build_path}/${build_type}_${COMPILER}/X64"

readonly script="${script:=${SCRIPT_DIR}/edk2_run_uefi_app.simics}"
readonly label="${label:=demo1}"
readonly uefi_app="${uefi_app:=Demo1_Example_App.efi}"
readonly disable_ptrenc="${disable_ptrenc:=0}"
readonly enable_integrity="${enable_integrity:=0}"
readonly disable_data_encryption="${disable_data_encryption:=0}"
readonly allow_scan="${allow_scan:=0}"
readonly magic="${magic:=0}"
readonly build_edk2=${build_edk2:=0}
readonly build_c3=${build_c3=0}
readonly log_dir="${EDK2_SRC_DIR}/logs"

cd "${PROJECT_DIR}"
mkdir -p "$log_dir"

if [[ "$build_edk2" == "1" ]]; then
    pushd "${SCRIPT_DIR}"
    ./build_uefi_simics.sh
    ./build_uefi_simics.sh
    popd
fi

if [[ $build_c3 == 1 ]]; then
    make clean
    make -j$(nproc) CC_EDK2=1
fi

logfile="$label.$uefi_app"
logfile="$logfile.ptrenc:$disable_ptrenc"
logfile="$logfile.int:$enable_integrity"
logfile="$logfile.data_enc:$disable_data_encryption"
logfile="$logfile.allow_scan:$allow_scan"

simics_args=(
    "$script"
    magic="$magic"
    bios_debug="$debug"
    disable_ptrenc="$disable_ptrenc"
    enable_integrity="$enable_integrity"
    disable_data_encryption="$disable_data_encryption"
    uefi_app="$uefi_app"
    allow_scan="$allow_scan"
    logfile="$log_dir/$logfile"
    app_debug="${app_debug:=0}"
)
# simics_args+=( -batch-mode )

if [[ -e "${build_path}/${uefi_app}" ]]; then
    echo ./simics "${simics_args[@]}"
    ./simics "${simics_args[@]}"
else
    echo "Cannot find: ${build_path}/${uefi_app}"
fi
