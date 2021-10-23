#!/usr/bin/env bash
if [ $# -ne 3 ]; then
    echo "Usage: $0 <n:unsigned> <β1:unsigned> <β2:unsigned>" 
    exit -1
fi

mkdir -p workloads  
cd build && make -j20
./build_concurrent_workload $1 1-ary cut $2 &
./build_concurrent_workload $1 2-ary cut $2  &
./build_concurrent_workload $1 "$(($1 - 1))"-ary cut $2  &
./build_concurrent_workload $1 random cut $2  &
./build_concurrent_workload $1 1-ary cut $3 &
./build_concurrent_workload $1 2-ary cut $3 &
./build_concurrent_workload $1 "$(($1 - 1))"-ary cut $3 &
./build_concurrent_workload $1 random cut $3 &
wait
