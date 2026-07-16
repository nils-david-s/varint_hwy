cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(varinthwy VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
option(BUILD_TEST "Build test" OFF)
set(DECODE_IMPL "HIGHWAY" CACHE STRING "Decoder implementation")

set(VALID_DECODE_IMPLS "HIGHWAY" "ARM_SVE_INTRINSIC")
if(NOT DECODE_IMPL IN_LIST VALID_DECODE_IMPLS)
    message(FATAL_ERROR "DECODE_IMP must be one of: ${VALID_DECODE_IMPLS}, got: ${DECODE_IMPL}")
endif()

add_compile_definitions(DECODE_IMPL_${DECODE_IMPL})
message("INFO: DECODE_IMPL: ${DECODE_IMPL}")

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
endif()

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    add_compile_options(
        -march=armv8-a+sve2
    )
endif()


#add_subdirectory(submodules/varint_rvv)

include_directories(
    ${CMAKE_SOURCE_DIR}/lib/include
    ${CMAKE_SOURCE_DIR}/submodules/highway
)

set(VARINT_RVV_C ${CMAKE_SOURCE_DIR}/submodules/varint_rvv/lib/src/varint_encode.c)
set(VARINT_HWY_CPP ${CMAKE_SOURCE_DIR}/lib/src/varint_decode_hwy.cpp)
set(VARINT_ARM_SVE ${CMAKE_SOURCE_DIR}/lib/src/varint_decode_sve.c)
set(TEST_CPP ${CMAKE_SOURCE_DIR}/test/test.cpp)


if(BUILD_TEST)
    if(DECODE_IMPL STREQUAL "HIGHWAY")
        add_library(varinthwy STATIC ${VARINT_HWY_CPP})
        add_executable(test_varint_hwy ${TEST_CPP} ${VARINT_RVV_C} ${VARINT_HWY_CPP})
        target_link_libraries(test_varint_hwy varinthwy -static)
    elseif(DECODE_IMPL STREQUAL "ARM_SVE_INTRINSIC")
        add_library(varintarmsve STATIC ${VARINT_ARM_SVE})
        add_executable(test_varint_hwy ${TEST_CPP} ${VARINT_RVV_C} ${VARINT_ARM_SVE})
        target_link_libraries(test_varint_hwy varintarmsve -static)
    endif()
    target_include_directories(
        test_varint_hwy PRIVATE
        ${CMAKE_SOURCE_DIR}/submodules/varint_rvv/lib/include
    )
endif()
