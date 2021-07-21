#!/bin/bash
set -e
set -x

mkdir -p run_on_local_results
declare -a list_of_workloads=("astar" "bzip" "dealII" "gcc" "gobmk" "h264ref" "hmmer" "lbm" "libquantum" "mcf" "milc" "namd" "omnetpp" "perlbench" "povray" "sjeng" "soplex" "sphinx" "xalancbmk")

for i in "${list_of_workloads[@]}"
do
  TRACE_ALLOCS=1 ./scripts/run_on_native.sh $i > run_on_local_results/$i.log 2>&1 &
done