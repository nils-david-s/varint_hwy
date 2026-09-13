add_executable(test_varint
    test.cpp
)

target_link_libraries(test_varint PRIVATE
    test_utils
    hwy
    varint_hwy_portable
    varint_scalar
    varint_encode
    varint_hwy_riscv
    varint_decode_vecshift
)

