add_library(varint_hwy_portable
    lib/include/varint_hwy_portable.h
    lib/src/varint_decode_hwy_portable.cpp
    lib/src/varint_decode_hwy_portable_early_branch.cpp
    lib/src/varint_decode_hwy_portable_nomask.cpp
    lib/src/varint_decode_hwy_portable_wmacclib.cpp
)
target_include_directories(varint_hwy_portable PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/include
)
target_link_libraries(varint_hwy_portable PRIVATE hwy)

