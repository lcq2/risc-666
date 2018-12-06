cmake_minimum_required(VERSION 3.12)
project(risc_666)

set(CMAKE_CXX_STANDARD 17)

#add_compile_definitions(DEBUG_SYSCALLS)
add_executable(risc_666 main.cpp elfloader.h elfloader.cpp rv_memory.h rv_memory.cpp rv_global.h rv_exceptions.h rv_cpu.h rv_cpu.cpp rv_bits.h newlib_syscalls.h newlib_trans.h newlib_trans.cpp)