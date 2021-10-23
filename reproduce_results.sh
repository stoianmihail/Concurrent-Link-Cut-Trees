mkdir -p build && cd build && cmake .. && make -j20 && cd ..
n=10000000
b1=100000
b2=1000000
./build_workload.sh $n $b1 $b2
./run_benchmark.sh $n $b1 $b2
