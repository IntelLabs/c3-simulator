#! /usr/bin/env bash

set -euxo pipefail

cwd=$(dirname "$(realpath "$0")")
prefix="${cwd}/llvm_install"

cmake_min_version="3.26.4"
cmake_bin_opt="${cwd}/../lib/cmake-3.26.4/bin/cmake"

link_jobs=1
compile_jobs="$(n=$(nproc); echo $((n-2 > 1 ? n-2 : 1)))"

# Should we build and install lldb too?
CC_LLVM_LLDB=${CC_LLVM_FULL:=0}

llvm_projects="clang;clang-tools-extra;lld;compiler-rt;lldb"
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
     ${cmake_bin} -G Ninja \
          -DCMAKE_BUILD_TYPE:STRING="Release" \
          -DCMAKE_INSTALL_PREFIX:FILEPATH="${prefix}" \
          -DLLVM_TARGETS_TO_BUILD:STRING="X86"  \
          -DLLVM_ENABLE_PROJECTS:STRING="${llvm_projects}" \
          -DLLVM_ENABLE_RUNTIMES:STRING="${llvm_runtimes}" \
          -DBUILD_SHARED_LIBS:BOOL="Off" \
          -DLLVM_PARALLEL_LINK_JOBS:STRING="${link_jobs}" \
          -DLLVM_PARALLEL_COMPILE_JOBS:STRING="${compile_jobs}" \
          -DLLVM_ENABLE_BINDINGS=Off \
          -DLLVM_BUILD_BENCHMARKS=Off \
          -DLLVM_BUILD_DOCS=Off \
          -DLLVM_BUILD_EXAMPLES=Off \
          -DLLVM_BUILD_INSTRUMENTED_COVERAGE=Off \
          -DLLVM_BUILD_TESTS=Off \
          -DLLVM_BUILD_TOOLS=On \
          -DLLVM_INSTALL_UTILS=On \
          "${srcdir}"
     ${cmake_bin} --build .
     ${cmake_bin} --install .
     popd
     echo "done building and installing LLVM"
}

make_llvm
