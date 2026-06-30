To build the project a toolchain file per target is needed in the project root directory.<br>
(toolchain-riscv.cmake, toolchain-arm.cmake, toolchain-x86_64.cmake)

Example for riscv:<br>
set(CMAKE_SYSTEM_NAME Linux)<br>
set(CMAKE_SYSTEM_PROCESSOR riscv64)<br>

set(CMAKE_C_COMPILER /usr/bin/riscv64-linux-gnu-gcc-15.1.0)<br>
set(CMAKE_CXX_COMPILER /usr/bin/riscv64-linux-gnu-c++)<br>
set(CMAKE_AR /usr/bin/riscv64-linux-gnu-ar)<br>
set(CMAKE_RANLIB /usr/bin/riscv64-linux-gnu-ranlib)
