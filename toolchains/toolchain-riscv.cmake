set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

set(CMAKE_C_COMPILER /usr/bin/riscv64-linux-gnu-gcc-15.1.0)
set(CMAKE_CXX_COMPILER /usr/bin/riscv64-linux-gnu-c++)
set(CMAKE_AR /usr/bin/riscv64-linux-gnu-ar)
set(CMAKE_RANLIB /usr/bin/riscv64-linux-gnu-ranlib)

set(CMAKE_C_FLAGS_INIT "-march=rv64gcv_zba_zbb_zbc_zbs_zkt_zfh_zcd_zca -mabi=lp64d")
set(CMAKE_CXX_FLAGS_INIT "-march=rv64gcv_zba_zbb_zbc_zbs_zkt_zfh_zcd_zca -mabi=lp64d")
