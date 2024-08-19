#! /usr/bin/env bash

# Copyright 2016-2024 Intel Corporation
# SPDX-License-Identifier: MIT

set -euo pipefail

cwd=$(dirname "$(realpath "$0")")
prefix="${cwd}/llvm_install"

cmake_min_version="3.26.4"
cmake_bin_opt="${cwd}/../lib/cmake-3.26.4/bin/cmake"

link_jobs=1
compile_jobs="$(n=$(nproc); echo $((n-2 > 1 ? n-2 : 1)))"

# Should we build and install lldb too?
CC_LLVM_LLDB=${CC_LLVM_LLDB:=0}

CC_LLVM_CONFIGURE_ONLY=${CC_LLVM_CONFIGURE_ONLY:=0}

llvm_projects="clang;clang-tools-extra;lld;compiler-rt"
llvm_runtimes="libunwind"
[[ $CC_LLVM_LLDB == 1 ]] && llvm_projects="${llvm_projects};lldb"

# Try to find CMake > cmake_min_version
cmake_bin=${cmake_bin:=""}
if [[ -z ${cmake_bin} ]]; then
     v="$(cmake --version | head -1 | cut -f3 -d" ")"
     mapfile -t sorted < <(printf "%s\n" "$v" "$cmake_min_version" | sort -V)
     if [[ ${sorted[0]} == "$cmake_min_version" ]]; then
          cmake_bin=cmake
     else
          if [[ -e ${cmake_bin_opt} ]]; then
               cmake_bin="${cmake_bin_opt}"
          else
               set +x
               cat <<EOF

Need cmake version >= ${cmake_min_version}

To install within project, run:
     make install-cmake

EOF
               exit 1
          fi
     fi
fi

[[ "$(echo "${MAKEFLAGS:=""}" | awk '{print $1}')" == *n* ]] && dry=1
dry=${dry:="0"}

make_llvm() {
     local srcdir="${cwd}/src/llvm"
     local builddir="${cwd}/llvm_build"

     cmake_args=()

     [[ -n ${CC:=""} ]] && cmake_args+=( "-DCMAKE_C_COMPILER=${CC}" )
     [[ -n ${CXX:=""} ]] && cmake_args+=( "-DCMAKE_CXX_COMPILER=${CXX}" )

     c3lib_path="$(cd "${cwd}"/../c3lib; pwd)"

     cmake_args+=(
          -DCMAKE_BUILD_TYPE=Release
          -DCMAKE_CXX_FLAGS="-I${c3lib_path}"
          -DCMAKE_C_FLAGS="-I${c3lib_path}"
          "-DCMAKE_INSTALL_PREFIX=${prefix}"
          -DLLVM_TARGETS_TO_BUILD=X86
          "-DLLVM_ENABLE_PROJECTS=${llvm_projects}"
          "-DLLVM_ENABLE_RUNTIMES=${llvm_runtimes}"
          -DBUILD_SHARED_LIBS=Off
          "-DLLVM_PARALLEL_LINK_JOBS=${link_jobs}"
          "-DLLVM_PARALLEL_COMPILE_JOBS=${compile_jobs}"
          -DLLVM_ENABLE_BINDINGS=Off
          -DLLVM_BUILD_BENCHMARKS=Off
          -DLLVM_BUILD_DOCS=Off
          -DLLVM_BUILD_EXAMPLES=Off
          -DLLVM_BUILD_INSTRUMENTED_COVERAGE=Off
          -DLLVM_BUILD_TESTS=Off
          -DLLVM_BUILD_TOOLS=On
          -DLLVM_INSTALL_UTILS=On
     )

     if [[ -e "${HOME}/.c3-llvm-ccache" ]]; then
          cmake_args+=(
               -DLLVM_CCACHE_BUILD=On
               -DLLVM_CCACHE_DIR="${HOME}/.c3-llvm-ccache"
          )
     fi

     if [[ $dry != 1 ]]; then
          rm -rf "${cwd:?}/llvm_install"/*
          mkdir -p "${prefix}"
          mkdir -p "${builddir}"
          pushd "${builddir}"
          ${cmake_bin} -G Ninja "${cmake_args[@]}" "${srcdir}"
          popd
     else
          echo rm -rf "${cwd:?}/llvm_install"/*
          echo mkdir -p "${prefix}"
          echo mkdir -p "${builddir}"
          echo pushd "${builddir}"
          echo ${cmake_bin} -G Ninja "${cmake_args[@]}" "${srcdir}"
          echo popd
     fi

     echo "done configuring LLVM build"

     if [[ $CC_LLVM_CONFIGURE_ONLY == 1 ]]; then
          return
     fi

     if [[ $dry != 1 ]]; then
          pushd "${builddir}"
          ${cmake_bin} -G Ninja "${cmake_args[@]}" "${srcdir}"
          ${cmake_bin} --build .
          ${cmake_bin} --install .
          popd
     else
          echo pushd "${builddir}"
          echo ${cmake_bin} --build .
          echo ${cmake_bin} --install .
          echo echo popd
     fi

     echo "done building and installing LLVM"
}

make_llvm
