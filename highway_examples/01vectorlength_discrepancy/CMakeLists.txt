add_executable(01vectorlength_discrepancy
    example/vectorlength_discrepancy.cpp
    lib/src/vectorlength_discrepancy.cpp
)

target_include_directories(01vectorlength_discrepancy PRIVATE
    lib/include
)

target_link_libraries(01vectorlength_discrepancy PRIVATE hwy)
