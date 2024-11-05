cmake_minimum_required(VERSION 3.29)
project(embedded_sim LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)


add_library(embedded_sim_lib STATIC
        src/proc/alu_private.c
        src/model/Register_private.c
        src/model/Instruction_private.c
        src/model/Instruction_private.c
        src/proc/Ipu_private.c
        src/proc/Cpu.h
        src/proc/Cpu_private.c
)

add_executable(embedded_sim main.c)

target_include_directories(embedded_sim_lib PUBLIC src)

target_link_libraries(embedded_sim embedded_sim_lib)

include(FetchContent)
enable_testing()
add_subdirectory(test)
add_subdirectory(src/parser)