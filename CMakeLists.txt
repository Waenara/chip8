cmake_minimum_required(VERSION 3.31)
project(chip8)

set(CMAKE_CXX_STANDARD 14)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(chip8
        main.cpp
        chip8.cpp
        lib/tinyfiledialogs/tinyfiledialogs.c
)

target_link_libraries(chip8 ${SDL2_LIBRARIES})
