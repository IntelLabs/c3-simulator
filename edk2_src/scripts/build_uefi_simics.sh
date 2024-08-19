#!/usr/bin/env bash
# Copyright 2023-2024 Intel Corporation
# SPDX-License-Identifier: MIT

# NOTE! Cannot set -u due to errors in the EDK2 scripts that need to be sourced!
set -eo pipefail

SCRIPT_PATH_NOLINK=$(readlink -f -- "${BASH_SOURCE[0]}")
SCRIPT_DIR="$( cd -- "$( dirname -- "${SCRIPT_PATH_NOLINK}" )" &> /dev/null && pwd )"
EDK2_SRC_DIR="$( cd -- "${SCRIPT_DIR}"/.. &> /dev/null && pwd )"
PROJECT_DIR="$( cd -- "${SCRIPT_DIR}"/../.. &> /dev/null && pwd )"

debug=${debug:=1}
COMPILER=${COMPILER:="GCC5"}
LLVM_PATH=${LLVM_PATH:="${PROJECT_DIR}/llvm/llvm_install/bin"}
LLVM_LD_LIBRARY_PATH=${LLVM_LD_LIBRARY_PATH:="${PROJECT_DIR}/llvm/llvm_install/lib"}

pushd "${EDK2_SRC_DIR}/edk2"

mkdir -p Conf
source edksetup.sh

source edksetup.sh BaseTools
make -C BaseTools

popd


build_once() {
    pushd "${EDK2_SRC_DIR}/edk2-platforms/Platform/Intel"
    debug=$debug "${SCRIPT_DIR}/GenConstantSectionOffsets.sh"
    if [[ $debug != 0 ]]; then
        PATH=$LLVM_PATH:$PATH LD_LIBRARY_PATH=$LLVM_LD_LIBRARY_PATH:$LD_LIBRARY_PATH python3 ./build_bios.py --platform BoardX58Ich10 --DEBUG -t $COMPILER
    else
        PATH=$LLVM_PATH:$PATH LD_LIBRARY_PATH=$LLVM_LD_LIBRARY_PATH:$LD_LIBRARY_PATH python3 ./build_bios.py --platform BoardX58Ich10 --RELEASE -t $COMPILER
    fi
    popd
}

build_once
build_once

set +x
echo DONE!
