add_library(varint_scalar
    lib/src/varint_decode_scalar_tail.c
    ${CMAKE_SOURCE_DIR}/submodules/varint_rvv/lib/src/varint_decode_scalar.c
)
target_include_directories(varint_scalar PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/include
)
