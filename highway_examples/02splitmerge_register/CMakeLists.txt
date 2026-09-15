add_library(02splitmerge_register
    lib/src/splitmerge_register.cpp
)

target_include_directories(02splitmerge_register PUBLIC
    lib/include
)

target_link_libraries(02splitmerge_register PRIVATE hwy)
