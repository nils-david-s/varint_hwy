add_library(varint_hwy_riscv
    lib/include/varint_hwy_riscv.h
    lib/src/varint_decode_hwy_riscv.cpp
    lib/src/varint_decode_hwy_riscv_wmacclib.cpp
    lib/src/varint_decode_hwy_riscv_nomask.cpp
    lib/src/varint_decode_hwy_riscv_mul_add.cpp
)
target_include_directories(varint_hwy_riscv PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/include
)

target_link_libraries(varint_hwy_riscv PRIVATE hwy)
