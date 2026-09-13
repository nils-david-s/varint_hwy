add_executable(benchmark_varint
    benchmark.cpp
)

target_link_libraries(benchmark_varint PRIVATE
    benchmark
    varint_hwy_portable
    varint_scalar
    varint_encode
)

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "riscv64")
    target_link_libraries(benchmark_varint PRIVATE
        varint_hwy_riscv
        varint_decode_vecshift
    )
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    target_link_libraries(benchmark_varint PRIVATE
        varint_arm
    )
endif()
