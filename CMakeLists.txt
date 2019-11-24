cmake_minimum_required(VERSION 3.15)
project(proj2_cse240a_clion C)

set(CMAKE_C_STANDARD 11)

include_directories(src)

add_executable(cache
        src/cache.c
        src/main.c)
