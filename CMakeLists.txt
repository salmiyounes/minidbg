cmake_minimum_required(VERSION 3.28.3)

project(minidbg)

add_executable(minidbg
    src/main.cpp
    src/breakpoint.cpp
    thirdparty/bestline/bestline.c 
    thirdparty/proc_maps_parser/pmparser.c
)

target_include_directories(minidbg
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/thirdparty/bestline
    ${PROJECT_SOURCE_DIR}/thirdparty/proc_maps_parser
)

target_compile_options(minidbg
    PRIVATE
    -g -Wall -Wextra -Wpedantic
)