add_executable(test_varint
    test.cpp
)

target_link_libraries(test_varint PRIVATE
    hwy
    varint_hwy_portable
    varint_scalar
    varint_encode
)

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "riscv64")
    target_link_libraries(test_varint PRIVATE
        varint_hwy_riscv
        varint_decode_vecshift
    )
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    target_link_libraries(test_varint PRIVATE
        varint_arm
    )
endif()

