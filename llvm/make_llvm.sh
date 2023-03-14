#! /usr/bin/env bash

set -euxo pipefail

cwd=$(dirname "$(realpath "$0")")
prefix="${cwd}/llvm_install"

link_jobs=1
compile_jobs="$(n=$(nproc); echo $((n-2 > 1 ? n-2 : 1)))"

# Should we build and install lldb?
CC_LLVM_LLDB=${CC_LLVM_LLDB:=0}

# NOTE: This seems to fail without clang
llvm_projects="clang;libunwind;compiler-rt;lldb"
# llvm_projects="${llvm_projects};libcxx;libcxxabi;lld"

# try to install cmake if we don't have it:
command -v cmake >/dev/null 2>&1 ||
    sudo apt install cmake

make_llvm() {
     local srcdir="${cwd}/src/llvm"
     local builddir="${cwd}/llvm_build"

     rm -rf "${cwd}/llvm_install"
     mkdir -p "${prefix}"
     mkdir -p "${builddir}"

     pushd "${builddir}"
     cmake -G Ninja \
          -DCMAKE_BUILD_TYPE:STRING="Release" \
          -DCMAKE_INSTALL_PREFIX:FILEPATH="${prefix}" \
          -DLLVM_TARGETS_TO_BUILD:STRING="X86"  \
          -DLLVM_ENABLE_PROJECTS:STRING="${llvm_projects}" \
          -DBUILD_SHARED_LIBS:BOOL="Off" \
          -DLLVM_PARALLEL_LINK_JOBS:STRING="${link_jobs}" \
          -DLLVM_PARALLEL_COMPILE_JOBS:STRING="${compile_jobs}" \
          -DLLVM_ENABLE_BINDINGS=Off \
          -DLLVM_BUILD_BENCHMARKS=Off \
          -DLLVM_BUILD_DOCS=Off \
          -DLLVM_BUILD_EXAMPLES=Off \
          -DLLVM_BUILD_INSTRUMENTED_COVERAGE=Off \
          -DLLVM_BUILD_TESTS=Off \
          -DLLVM_BUILD_TOOLS=Off \
          -DLLVM_INSTALL_UTILS=Off \
          "${srcdir}"
     ninja install-unwind-stripped
     if [[ $CC_LLVM_LLDB == 1 ]]; then
          ninja install-lldb
     fi
     popd
     echo "done building and installing LLVM"
}

make_llvm
