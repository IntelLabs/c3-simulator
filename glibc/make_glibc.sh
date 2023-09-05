#!/bin/bash
set -euo pipefail
# set -x

cwd=`realpath $0 ` # Get the path to this file
cwd=`dirname ${cwd}`

readonly glibc_build="${cwd}/glibc-2.30_build"
readonly glibc_install="${cwd}/glibc-2.30_install"
readonly build_conf="${cwd}/glibc-2.30_build/c3_build_config"

prefix="$glibc_install"
# jobs="-j8"
jobs="-j$(n=$(nproc); echo $((n-2 > 1 ? n-2 : 1)))"

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

readonly build_conf_string=$(cat <<EOF
USE_CC_ISA=$USE_CC_ISA
CC_USE_SYSCALL_SHIMS=$CC_USE_SYSCALL_SHIMS
CC_NO_ICV_ENABLE=$CC_NO_ICV_ENABLE
EOF
)

# Clean build and install directory if C3-specific configuration has changed
if ! [[ -e $build_conf ]] || [[ "$(cat "$build_conf")" != "$build_conf_string" ]]; then
    echo "Deleting old glibc build and install directory"
    rm -rf "$glibc_build"
    rm -rf "$glibc_install"
fi

cd "${cwd}"
mkdir -p glibc-2.30_install
mkdir -p glibc-2.30_build

# Store configuration so we we can clean if this changes
echo "$build_conf_string" > "$build_conf"

CPPFLAGS=${CPPFLAGS:=""}
CPPFLAGS="${CPPFLAGS} -I${cwd}/../malloc -DCC"
CFLAGS="-fcf-protection=none -g -O2"

if [[ ${USE_CC_ISA} == "1" ]]; then
    CPPFLAGS="${CPPFLAGS} -DUSE_CC_ISA"
    CFLAGS="${CFLAGS} -DUSE_CC_ISA"
fi

if [[ ${CC_USE_SYSCALL_SHIMS} == "1" ]]; then
    CPPFLAGS="${CPPFLAGS} -DCC_USE_SYSCALL_SHIMS"
    CFLAGS="${CFLAGS} -DCC_USE_SYSCALL_SHIMS"
fi

if [[ ${CC_NO_ICV_ENABLE} == "1" ]]; then
    CPPFLAGS="${CPPFLAGS} -DCC_NO_ICV_ENABLE"
    CFLAGS="${CFLAGS} -DCC_NO_ICV_ENABLE"
fi


cd glibc-2.30_build
./../src/configure prefix="${prefix}" \
        --enable-multi-arch=no \
        CPPFLAGS="${CPPFLAGS}" \
        CFLAGS="${CFLAGS}"

make ${jobs}

# ld.so.conf is required by make install, so create an empty file
mkdir -p "${prefix}/etc"
touch "${prefix}/etc/ld.so.conf"

make install

cd ..

rm -f "${prefix}/lib/libstdc++.so.6"
rm -f "${prefix}/lib/libgcc_s.so.1"
ln -s "${system_libstdcpp}" "${prefix}/lib/libstdc++.so.6"
ln -s "${system_libgcc}" "${prefix}/lib/libgcc_s.so.1"
echo "DONE BUILDING GLIBC"
