#!/usr/bin/env bash
if [ $# -ne 3 ]; then
    echo "Usage: $0 <n:unsigned> <β1:unsigned> <β2:unsigned>" 
    exit -1
fi

cd build && make -j20
./bench ../workloads/cut-1-ary-$2-$1.bin
./bench ../workloads/cut-2-ary-$2-$1.bin
./bench ../workloads/cut-"$(($1 - 1))"-ary-$2-$1.bin
./bench ../workloads/cut-random-$2-$1.bin
./bench ../workloads/cut-1-ary-$3-$1.bin
./bench ../workloads/cut-2-ary-$3-$1.bin
./bench ../workloads/cut-"$(($1 - 1))"-ary-$3-$1.bin
./bench ../workloads/cut-random-$3-$1.bin

FS="1-ary 2-ary "$(($1 - 1))"-ary random"
TS="2 4 8 16 32 48 56 84 112"

echo 'Benchmarking..'
echo $FS
echo $TS

for F in $FS; do
  for T in $TS; do
    ./concurrent_bench ../workloads/cut-$F-$2-$1.bin $T 8
    ./concurrent_bench ../workloads/cut-$F-$3-$1.bin $T 8
    ./concurrent_bench ../workloads/cut-$F-$2-$1.bin $T 16
    ./concurrent_bench ../workloads/cut-$F-$3-$1.bin $T 16
    ./concurrent_bench ../workloads/cut-$F-$2-$1.bin $T 32
    ./concurrent_bench ../workloads/cut-$F-$3-$1.bin $T 32
  done;
done;
