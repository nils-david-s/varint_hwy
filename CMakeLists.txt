cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(varinthwy VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
set(BUILD_TEST CACHE BOOL OFF)

add_compile_options(
    -march=rv64gcv_zba_zbb_zbc_zbs_zkt_zfh_zcd_zca 
    -mabi=lp64d 
    -O3 
    -static 
    -Wall 
    -Werror
)

add_subdirectory(submodules/varint_rvv)

include_directories(
    ${CMAKE_SOURCE_DIR}/lib/include
    ${CMAKE_SOURCE_DIR}/submodules/highway
)

set(VARINT_RVV_C ${CMAKE_SOURCE_DIR}/submodules/varint_rvv/lib/src/varint_encode.c)
set(VARINT_HWY_CPP ${CMAKE_SOURCE_DIR}/lib/src/varint_decode_hwy.cpp)
set(TEST_CPP ${CMAKE_SOURCE_DIR}/test/test.cpp)

add_library(varinthwy STATIC ${VARINT_HWY_CPP})

if(BUILD_TEST)
    add_executable(test_varint_hwy ${TEST_CPP} ${VARINT_RVV_C} ${VARINT_HWY_CPP})
    target_link_libraries(test_varint_hwy varinthwy -static)
    target_include_directories(
        test_varint_hwy PRIVATE
        ${CMAKE_SOURCE_DIR}/submodules/varint_rvv/lib/include
    )
endif()
