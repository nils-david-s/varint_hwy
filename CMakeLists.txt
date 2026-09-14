cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(varinthwy VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable Benchmark's tests" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "Disable Benchmark's GTest tests")

set(HWY_ENABLE_CONTRIB OFF CACHE BOOL "Include contrib/")
set(HWY_ENABLE_EXAMPLES OFF CACHE BOOL "Build examples")
set(HWY_ENABLE_TESTS OFF CACHE BOOL "Enable HWY tests")

set(OPT_LEVEL "O3" CACHE STRING "Optimization level")
add_compile_options(
    -${OPT_LEVEL}
    -Wall
    -Werror
)

add_link_options(
    -static
)

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "riscv64")
    add_compile_definitions(RISCV)
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    add_compile_definitions(SVE2)
endif()

add_subdirectory(encode)
add_subdirectory(submodules/google-benchmark)
add_subdirectory(submodules/highway)
add_subdirectory(decode)
add_subdirectory(benchmark)
add_subdirectory(test)
add_subdirectory(highway_examples)
