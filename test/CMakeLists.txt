add_subdirectory(utils)

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "riscv64")
    add_subdirectory(riscv)
    target_link_libraries(test_varint PRIVATE
        varint_hwy_riscv
        varint_decode_vecshift
    )
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    add_subdirectory(arm)
    target_link_libraries(test_varint PRIVATE
        varint_arm
    )
else()
    add_subdirectory(x86_64)
endif()

