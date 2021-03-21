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
    ${PROJECT_SOURCE_DIR}/Source/File.cc
    ${PROJECT_SOURCE_DIR}/Source/Generation.cc
    ${PROJECT_SOURCE_DIR}/Source/Parsing.cc
    ${PROJECT_SOURCE_DIR}/Source/Tokenization.cc
)
target_include_directories(simple-mips-asm PUBLIC ${PROJECT_SOURCE_DIR}/Public)

# Executable definitions
add_executable(runfile ${PROJECT_SOURCE_DIR}/Source/Main.cc)
target_link_libraries(runfile simple-mips-asm)

# Unit tests
option(SIMPLE_MIPS_ASM_TEST "Enable unit tests" OFF)
if (SIMPLE_MIPS_ASM_TEST)
    enable_testing()
    find_package(GTest)
    function(add_simple_mips_asm_test TEST_NAME)
        set(FILE_NAME ${TEST_NAME})
        set(EXE_NAME ${TEST_NAME})
        
        string(REGEX REPLACE "([^A-Z\-])([A-Z][A-Z]+)([A-Z][a-z])" "\\1-\\2-\\3" EXE_NAME "${EXE_NAME}")
        string(REGEX REPLACE "([A-Z]+)$" "-\\1" EXE_NAME "${EXE_NAME}")
        string(REGEX REPLACE "([^-])([A-Z][a-z]+)" "\\1-\\2" EXE_NAME "${EXE_NAME}")
        string(TOLOWER "${EXE_NAME}" EXE_NAME)
        
        add_executable(${EXE_NAME} "Tests/${FILE_NAME}.cc")
        target_link_libraries(${EXE_NAME} GTest::gtest_main simple-mips-asm)
        add_test(NAME ${TEST_NAME} COMMAND ${EXE_NAME})
        
        unset(FILE_NAME)
        unset(EXE_NAME)
        unset(TEST_NAME)
    endfunction()

    add_simple_mips_asm_test(TokenizationTest)
    add_simple_mips_asm_test(ParsingTest)
endif()