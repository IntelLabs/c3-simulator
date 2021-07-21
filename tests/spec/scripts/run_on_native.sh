#!/bin/bash
set -e
set -x

# How to use:
# GLIBC must be compiled such that encoded pointers will not be used by the workload. TODO: Pull the flag for doing this in from bin-trace branch.
# You must verify that the path to the two libraries linked into your glibc install below are correct. These libraries are not compiled as part of glibc and you should link the system default copy of these .so's to the install `ldd` search path. You can find the true location of these binaries by using your system default `ldd` on most c/c++ programs.
# cd to the spec directory.
# Run the script with "./run_on_native astar"
# Set the TRACE_ALLOCS environment variable to save allocation traces.

cwd=`realpath $0 ` # Get the path to this file
cwd=`dirname ${cwd}`
glibc_inst=${cwd}/../../../glibc/glibc-2.30_install/lib
cd ${cwd}/..
perf_cmd="/usr/bin/perf stat -e cpu-cycles,instructions,branch-instructions,branch-misses,cache-references,cache-misses,page-faults,context-switches,L1-dcache-loads,L1-dcache-load-misses,L1-dcache-stores,dTLB-load-misses,dTLB-stores,dTLB-store-misses -o"
patchelf_cmd="/usr/bin/patchelf --set-interpreter ${glibc_inst}/ld-2.30.so --set-rpath ${glibc_inst}"


benchmark=$1

ln -sfn /usr/lib/x86_64-linux-gnu/libstdc++.so.6 ${glibc_inst}
ln -sfn /usr/lib/x86_64-linux-gnu/libgomp.so.1 ${glibc_inst}
ln -sfn /lib/x86_64-linux-gnu/libgcc_s.so.1 ${glibc_inst}

cd ${benchmark}

mkdir -p build runlocal

# match on files within directories instead of deleting entire directories,
# since the latter approach often fails on NFS:
rm -rf build/* runlocal/*
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ../src -G Ninja
ninja
cd ..

cp -r run/* runlocal
cp -p build/${benchmark} runlocal/
cd runlocal
cp -p ${benchmark} ${benchmark}_glibc_230
${patchelf_cmd} ${benchmark}_glibc_230

if [[ -z "${TRACE_ALLOCS}" ]]; then
    ./run.sh ${benchmark}_glibc_230
    LIM_ENABLED="1" ./run.sh ${benchmark}_glibc_230
else
    CC_TRACE="${benchmark}.nolim" ./run.sh ${benchmark}_glibc_230
    LIM_ENABLED="1" CC_TRACE="${benchmark}.lim" ./run.sh ${benchmark}_glibc_230
fi
