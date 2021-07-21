#!/bin/bash
set -ex

cwd=`realpath $0 ` # Get the path to this file
cwd=`dirname ${cwd}`
cd ${cwd}

mkdir -p glibc-2.30_install
mkdir -p glibc-2.30_build

CPPFLAGS="${CPPFLAGS} -I${cwd}/../malloc -DCC"
CFLAGS="-fcf-protection=none -g -O2"

cd glibc-2.30_build
./../src/configure prefix=${cwd}/glibc-2.30_install CC=gcc-8 CPPFLAGS="${CPPFLAGS}" CFLAGS="${CFLAGS}"

make -j8 
make install

cd ..

rm -f ${cwd}/glibc-2.30_install/lib/libstdc++.so.6
rm -f ${cwd}/glibc-2.30_install/lib/libgcc_s.so.1
ln -s /usr/lib/x86_64-linux-gnu/libstdc++.so.6   ${cwd}/glibc-2.30_install/lib/libstdc++.so.6
ln -s /usr/lib/x86_64-linux-gnu/libgcc_s.so.1         ${cwd}/glibc-2.30_install/lib/libgcc_s.so.1
echo "DONE BUILDING GLIBC"
