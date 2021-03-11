# Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.13)
project(simple-mips-asm)
enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Library definitions
add_library(simple-mips-asm STATIC
    ${PROJECT_SOURCE_DIR}/Source/Assembler.cc
    ${PROJECT_SOURCE_DIR}/Source/Parsing.cc
    ${PROJECT_SOURCE_DIR}/Source/Tokenization.cc
)
target_include_directories(simple-mips-asm PUBLIC ${PROJECT_SOURCE_DIR}/Public)

# Executable definitions
add_executable(runfile ${PROJECT_SOURCE_DIR}/Source/Main.cc)
target_link_libraries(runfile simple-mips-asm)

# Unit tests
