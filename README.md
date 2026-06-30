To build the project a toolchain file per target is needed.

Example for riscv:
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

set(CMAKE_C_COMPILER /usr/bin/riscv64-linux-gnu-gcc-15.1.0)
set(CMAKE_CXX_COMPILER /usr/bin/riscv64-linux-gnu-c++)
set(CMAKE_AR /usr/bin/riscv64-linux-gnu-ar)
set(CMAKE_RANLIB /usr/bin/riscv64-linux-gnu-ranlib)
