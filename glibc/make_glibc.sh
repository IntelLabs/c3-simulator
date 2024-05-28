#!/bin/bash
set -euo pipefail
# set -x

cwd=`realpath $0 ` # Get the path to this file
cwd=`dirname ${cwd}`

readonly glibc_build="${cwd}/glibc-2.30_build"
readonly glibc_install="${cwd}/glibc-2.30_install"
readonly build_conf="${cwd}/glibc-2.30_build/c3_build_config"

prefix="$glibc_install"

system_libstdcpp="/usr/lib/x86_64-linux-gnu/libstdc++.so.6"
[[ -e "${system_libstdcpp}" ]] || system_libstdcpp="/usr/lib64/libstdc++.so.6"
[[ -e "${system_libstdcpp}" ]] || \
    (echo "Cannot find $(basename ${system_libstdcpp})" && exit 1)

system_libgcc="/usr/lib/x86_64-linux-gnu/libgcc_s.so.1"
[[ -e "${system_libgcc}" ]] || system_libgcc="/usr/lib64/libgcc_s.so.1"
[[ -e "${system_libgcc}" ]] || \
    (echo "Cannot find $(basename ${system_libgcc})" && exit 1)

# C3-specific options passed as macro defines to build
readonly USE_CC_ISA=${USE_CC_ISA:="1"}
readonly CC_USE_SYSCALL_SHIMS=${CC_USE_SYSCALL_SHIMS:="1"}
readonly CC_NO_ICV_ENABLE=${CC_NO_ICV_ENABLE:="0"}
readonly CC_CA_STACK_ENABLE=${CC_CA_STACK_ENABLE:="1"}
readonly GLIBC_MULTIARCH=${GLIBC_MULTIARCH:="0"}
readonly CC_NO_WRAP_ENABLE=${CC_NO_WRAP_ENABLE:="0"}

# Set MAKEFLAGS to emtpy string if it isn't defined
MAKEFLAGS=${MAKEFLAGS:=""}

[[ "$(echo "$MAKEFLAGS" | awk '{print $1}')" == *n* ]] && dry=1
dry=${dry:="0"}

# Build conf string for build directory to enable conditionally cleaning build
build_conf_string=$(cat <<EOF
USE_CC_ISA=$USE_CC_ISA
CC_USE_SYSCALL_SHIMS=$CC_USE_SYSCALL_SHIMS
CC_NO_ICV_ENABLE=$CC_NO_ICV_ENABLE
CC_CA_STACK_ENABLE=$CC_CA_STACK_ENABLE
GLIBC_MULTIARCH=$GLIBC_MULTIARCH
CC_NO_WRAP_ENABLE=$CC_NO_WRAP_ENABLE
EOF
)
readonly build_conf_string

# Clean build and install directory if C3-specific configuration has changed
if ! [[ -e $build_conf ]] || [[ "$(cat "$build_conf")" != "$build_conf_string" ]]; then
    echo "Deleting old glibc build and install directory"
    [[ $dry == 1 ]] || rm -rf "${glibc_build:?}"/*
    [[ $dry == 1 ]] || rm -rf "${glibc_install:?}"/*
fi

echo "Creating install and build directories"
[[ $dry == 1 ]] || {
    mkdir -p "${cwd}/glibc-2.30_install"
    mkdir -p "${cwd}/glibc-2.30_build"
}

# Store configuration so we we can clean if this changes
echo "$build_conf_string" > "$build_conf"

CC=${CC:="gcc-9"}
if [[ "$(${CC} --version | head -n1)" =~ gcc ]]; then
    GCC_VERSION=$(${CC} --version | head -n1 | awk '{print $3}' | sed 's/\..*//')
    echo "Compiler is gcc $GCC_VERSION"
elif [[ "$(${CC} --version | head -n1)" =~ clang ]]; then
    # The flags here assume clang / gcc flags are interchangeable...
    GCC_VERSION=$(${CC} --version | head -n1 | sed 's/.*version //' | sed 's/\..*//')
    echo "Compiler is clang $GCC_VERSION based on\$CC, but need gcc for glibc,"
    echo "setting compiler to gcc-9"
    CC="gcc-9"
    GCC_VERSION=9
else
    echo -e "Unrecognized compiler: ${CC}\n${CC} --version:"
    ${CC} --version
    exit 1
fi

CPPFLAGS=${CPPFLAGS:=""}
CPPFLAGS="${CPPFLAGS} -I${cwd}/../malloc -DCC"
CFLAGS=( -fcf-protection=none -g -O2 )
if (( GCC_VERSION >= 10 )); then
    CFLAGS+=(
        -Wno-error=zero-length-bounds
        -Wno-error=maybe-uninitialized
        -Wno-zero-length-bounds
        -Wno-maybe-uninitialized
    )
fi
if (( GCC_VERSION >= 11 )); then
    # Flags to build with GCC 11
    CFLAGS+=(
        -Wno-error=array-parameter
        -Wno-error=stringop-overflow
        -Wno-error=stringop-overread
        -Wno-error=array-bounds
        -Wno-array-parameter
        -Wno-stringop-overflow
        -Wno-stringop-overread
        -Wno-array-bounds
    )
fi
if (( GCC_VERSION >= 12 )); then
    CFLAGS+=(
        -Wno-error=use-after-free
        -Wno-error=format-overflow
        -Wno-use-after-free
        -Wno-format-overflow
    )
fi

if [[ ${USE_CC_ISA} == "1" ]]; then
    CPPFLAGS="${CPPFLAGS} -DUSE_CC_ISA"
    CFLAGS+=( -DUSE_CC_ISA )
fi

if [[ ${CC_USE_SYSCALL_SHIMS} == "1" ]]; then
    CPPFLAGS="${CPPFLAGS} -DCC_USE_SYSCALL_SHIMS"
    CFLAGS+=( -DCC_USE_SYSCALL_SHIMS )
fi

if [[ ${CC_NO_ICV_ENABLE} == "1" ]]; then
    CPPFLAGS="${CPPFLAGS} -DCC_NO_ICV_ENABLE"
    CFLAGS+=( -DCC_NO_ICV_ENABLE )
fi

if [[ ${CC_CA_STACK_ENABLE} == "1" ]]; then
    CPPFLAGS="${CPPFLAGS} -DCC_CA_STACK_ENABLE"
    CFLAGS+=( -DCC_CA_STACK_ENABLE )
fi

if [[ ${CC_NO_WRAP_ENABLE} == "1" ]]; then
    CPPFLAGS="${CPPFLAGS} -DCC_NO_WRAP_ENABLE"
    CFLAGS="${CFLAGS} -DCC_NO_WRAP_ENABLE"
fi

configure_args=(
    prefix="${prefix}"
    --without-selinux
    CPPFLAGS="${CPPFLAGS}"
    CFLAGS="${CFLAGS[*]}"
    CC="$CC"
)

# Enable mult-arch if GLIBC_MULTIARCH
[[ "${GLIBC_MULTIARCH}" -eq 1 ]] || configure_args+=( --enable-multi-arch=no )

echo ./../src/configure "${configure_args[@]}"
[[ $dry == 1 ]] || {
    pushd "${cwd}/glibc-2.30_build"
    ./../src/configure "${configure_args[@]}"
    popd
}

make_args=()

# Add `-j` flag to make unless we're running a sub-make, in which case the make
# jobserver will orchestrate things for us.
if [[ -z "$MAKEFLAGS" ]]; then
    # Set -j based on number of cores available
    jobs=${jobs:="-j$(n=$(nproc); echo $((n-2 > 1 ? n-2 : 1)))"}
    make_args+=( "${jobs}" )
fi

echo "make -C ${cwd}/glibc-2.30_build " "${make_args[@]}"
[[ $dry == 1 ]] || make -C "${cwd}/glibc-2.30_build" "${make_args[@]}"

# ld.so.conf is required by make install, so create an empty file
echo "Create empty ${prefix}/etc/ld.so.conf"
[[ $dry == 1 ]] || {
    mkdir -p "${prefix}/etc"
    touch "${prefix}/etc/ld.so.conf"
}

echo "make -C ${cwd}/glibc-2.30_build install"
[[ $dry == 1 ]] || make -C "${cwd}/glibc-2.30_build" install

echo "Linking ${system_libstdcpp} ${system_libgcc} to ${prefix}/lib"
[[ $dry == 1 ]] || {
    rm -f "${prefix}/lib/libstdc++.so.6"
    rm -f "${prefix}/lib/libgcc_s.so.1"
    ln -s "${system_libstdcpp}" "${prefix}/lib/libstdc++.so.6"
    ln -s "${system_libgcc}" "${prefix}/lib/libgcc_s.so.1"
}
echo "DONE BUILDING GLIBC"
