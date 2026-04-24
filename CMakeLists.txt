cmake_minimum_required(VERSION 3.28.3)

project(minidbg)

add_executable(minidbg
    src/main.cpp
    thirdparty/bestline/bestline.c 
)

target_include_directories(minidbg
    PRIVATE
    ${PROJECT_SOURCE_DIR}/thirdparty/bestline 
)

target_compile_options(minidbg
    PRIVATE
    -g -Wall -Wextra -Wpedantic
)