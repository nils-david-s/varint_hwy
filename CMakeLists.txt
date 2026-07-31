cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(varinthwy VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
option(BUILD_TEST "Build test" OFF)
option(BUILD_BENCHMARK "Build benchmark" OFF)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable Benchmark's tests" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOT "Disable Benchmark's GTest tests")

add_compile_options(
    -O3
    -static 
    -Wall 
    -Werror
)

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "riscv64")
    add_compile_options(
        -march=rv64gcv_zba_zbb_zbc_zbs_zkt_zfh_zcd_zca
        -mabi=lp64d
    )
    add_compile_definitions(RISCV)
endif()

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    add_compile_options(
        -march=armv8-a+sve2
    )
    add_compile_definitions(SVE2)
endif()


#add_subdirectory(submodules/varint_rvv)

include_directories(
    ${CMAKE_SOURCE_DIR}/lib/include
    ${CMAKE_SOURCE_DIR}/submodules/highway
    ${CMAKE_SOURCE_DIR}/submodules/google-benchmark/include
)

set(VARINT_RVV_ENCODE_C ${CMAKE_SOURCE_DIR}/submodules/varint_rvv/lib/src/varint_encode.c)
set(VARINT_RVV_SCALAR_C ${CMAKE_SOURCE_DIR}/submodules/varint_rvv/lib/src/varint_decode_scalar.c)
set(VARINT_RVV_VECSHIFT_C ${CMAKE_SOURCE_DIR}/submodules/varint_rvv/lib/src/varint_decode_vecshift.c)
set(VARINT_RVV_SCALAR_TAIL_C ${CMAKE_SOURCE_DIR}/lib/src/varint_decode_scalar_tail.c)
set(VARINT_HWY_PORTABLE ${CMAKE_SOURCE_DIR}/lib/src/varint_decode_hwy_portable.cpp)
set(VARINT_HWY_RISCV ${CMAKE_SOURCE_DIR}/lib/src/varint_decode_hwy_riscv.cpp)
set(VARINT_HWY_RISCV_MUL_ADD ${CMAKE_SOURCE_DIR}/lib/src/varint_decode_hwy_riscv_mul_add.cpp)
set(VARINT_ARM_SVE ${CMAKE_SOURCE_DIR}/lib/src/varint_decode_sve.c)

set(TEST_CPP ${CMAKE_SOURCE_DIR}/test/test.cpp)
set(BENCHMARK_CPP ${CMAKE_SOURCE_DIR}/benchmark/benchmark.cpp)

if(BUILD_TEST)
    add_library(varinthwyportable STATIC ${VARINT_HWY_PORTABLE})
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "riscv64")
        add_library(varinthwyriscv STATIC ${VARINT_HWY_RISCV})
        add_executable(test_varint_hwy ${TEST_CPP} ${VARINT_RVV_ENCODE_C} ${VARINT_HWY_PORTABLE} ${VARINT_HWY_RISCV} ${VARINT_RVV_VECSHIFT_C})
        target_link_libraries(test_varint_hwy varinthwyportable -static)
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
        add_library(varintarmsve STATIC ${VARINT_ARM_SVE})
        add_executable(test_varint_hwy ${TEST_CPP} ${VARINT_RVV_ENCODE_C} ${VARINT_HWY_PORTABLE} ${VARINT_ARM_SVE})
        target_link_libraries(test_varint_hwy varintarmsve -static)
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        add_executable(test_varint_hwy ${TEST_CPP} ${VARINT_RVV_ENCODE_C} ${VARINT_HWY_PORTABLE})
        target_link_libraries(test_varint_hwy -static)
    endif()
    target_include_directories(
        test_varint_hwy PRIVATE
        ${CMAKE_SOURCE_DIR}/submodules/varint_rvv/lib/include
    )
endif()
if(BUILD_BENCHMARK)
    add_subdirectory(submodules/google-benchmark)
    add_library(varinthwyportable STATIC ${VARINT_HWY_PORTABLE})
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "riscv64")
        add_library(varinthwyriscv STATIC ${VARINT_HWY_RISCV})
        add_executable(benchmark_varint ${BENCHMARK_CPP} ${VARINT_RVV_ENCODE_C} ${VARINT_RVV_SCALAR_C} ${VARINT_RVV_VECSHIFT_C} ${VARINT_HWY_PORTABLE} ${VARINT_HWY_RISCV} ${VARINT_HWY_RISCV_MUL_ADD} ${VARINT_RVV_SCALAR_TAIL_C})
        target_link_libraries(benchmark_varint varinthwyportable benchmark -static)
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
        add_library(varintarmsve STATIC ${VARINT_ARM_SVE})
        add_executable(benchmark_varint ${BENCHMARK_CPP} ${VARINT_RVV_ENCODE_C} ${VARINT_RVV_SCALAR_C} ${VARINT_HWY_PORTABLE} ${VARINT_ARM_SVE} ${VARINT_RVV_SCALAR_TAIL_C})
        target_link_libraries(benchmark_varint varintarmsve benchmark -static)
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        add_executable(benchmark_varint ${BENCHMARK_CPP} ${VARINT_RVV_ENCODE_C} ${VARINT_RVV_SCALAR_C} ${VARINT_HWY_PORTABLE} ${VARINT_RVV_SCALAR_TAIL_C})
        target_link_libraries(benchmark_varint benchmark -static)
    endif()
    target_include_directories(
        benchmark_varint PRIVATE
        ${CMAKE_SOURCE_DIR}/submodules/varint_rvv/lib/include
    )
endif()
