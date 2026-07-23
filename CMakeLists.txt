cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(varinthwy VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
option(BUILD_TEST "Build test" OFF)


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
)

set(VARINT_RVV_C ${CMAKE_SOURCE_DIR}/submodules/varint_rvv/lib/src/varint_encode.c)
set(VARINT_HWY_PORTABLE ${CMAKE_SOURCE_DIR}/lib/src/varint_decode_hwy_portable.cpp)
set(VARINT_HWY_RISCV ${CMAKE_SOURCE_DIR}/lib/src/varint_decode_hwy_riscv.cpp)
set(VARINT_ARM_SVE ${CMAKE_SOURCE_DIR}/lib/src/varint_decode_sve.c)
set(TEST_CPP ${CMAKE_SOURCE_DIR}/test/test.cpp)


if(BUILD_TEST)
    add_library(varinthwyportable STATIC ${VARINT_HWY_PORTABLE})
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "riscv64")
        add_library(varinthwyriscv STATIC ${VARINT_HWY_RISCV})
        add_executable(test_varint_hwy ${TEST_CPP} ${VARINT_RVV_C} ${VARINT_HWY_PORTABLE} ${VARINT_HWY_RISCV})
        target_link_libraries(test_varint_hwy varinthwyportable -static)
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
        add_library(varintarmsve STATIC ${VARINT_ARM_SVE})
        add_executable(test_varint_hwy ${TEST_CPP} ${VARINT_RVV_C} ${VARINT_HWY_PORTABLE} ${VARINT_ARM_SVE})
        target_link_libraries(test_varint_hwy varintarmsve -static)
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        add_executable(test_varint_hwy ${TEST_CPP} ${VARINT_RVV_C} ${VARINT_HWY_PORTABLE})
    endif()
    target_include_directories(
        test_varint_hwy PRIVATE
        ${CMAKE_SOURCE_DIR}/submodules/varint_rvv/lib/include
    )
endif()
