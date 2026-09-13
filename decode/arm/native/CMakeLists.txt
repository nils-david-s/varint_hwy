add_library(varint_arm
    lib/include/varint_arm.h
    lib/src/varint_decode_sve.c
    lib/src/varint_decode_sve_early_branch.c
)
target_include_directories(varint_arm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/include
)
target_link_libraries(varint_arm PRIVATE hwy)

