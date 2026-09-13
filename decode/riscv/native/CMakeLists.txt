add_library(varint_decode_vecshift
    ${CMAKE_SOURCE_DIR}/submodules/varint_rvv/lib/src/varint_decode_vecshift.c
)
target_include_directories(varint_decode_vecshift PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

