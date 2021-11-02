cmake_minimum_required(VERSION 3.10)
project(concurrent-lct)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3 -Wall -Wextra -O3")

find_package(Threads REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)

file(GLOB INCLUDE_H "include/*.hpp")
set(BENCH_FILES bench.cc)
set(TREE_DEBUGGER_FILES tree_debugger.cc)
set(CONCURRENT_BENCH_FILES concurrent_bench.cc)
set(CONCURRENT_WORKLOAD_FILES build_concurrent_workload.cc)

add_executable(bench ${INCLUDE_H} ${BENCH_FILES})
target_link_libraries(bench pthread)
add_executable(tree_debugger ${INCLUDE_H} ${TREE_DEBUGGER_FILES})
target_link_libraries(tree_debugger pthread)
add_executable(concurrent_bench ${INCLUDE_H} ${CONCURRENT_BENCH_FILES})
target_link_libraries(concurrent_bench pthread)
add_executable(build_concurrent_workload ${INCLUDE_H} ${CONCURRENT_WORKLOAD_FILES})
target_link_libraries(build_concurrent_workload pthread)
