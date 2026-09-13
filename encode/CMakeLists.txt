add_library(varint_encode
    ${CMAKE_SOURCE_DIR}/submodules/varint_rvv/lib/src/varint_encode.c
)
target_include_directories(varint_encode PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

