if(CMAKE_SYSTEM_PROCESSOR STREQUAL "riscv64")
    add_subdirectory(01vectorlength_discrepancy)
    add_subdirectory(02splitmerge_register)
endif()
