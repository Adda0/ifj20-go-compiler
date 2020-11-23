cmake_minimum_required(VERSION 3.12)
project(ifj20_go_compiler CXX C)
enable_testing()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

# ------- GoogleTest -------
# Download and unpack googletest at configure time
configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)
if (result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif ()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)
if (result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif ()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
        ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
        EXCLUDE_FROM_ALL)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

# ------- App -------
include_directories(src)
add_executable(Compiler
        src/compiler.h src/compiler.c
        src/mutable_string.h src/mutable_string.c
        src/stderr_message.h src/stderr_message.c
        src/scanner.h src/scanner_static.h src/scanner.c
        src/parser.h src/parser.c
        src/precedence_parser.h src/precedence_parser.c
        src/stacks.h src/stacks.c
        src/control_flow.h src/control_flow.c
        src/optimiser.h src/optimiser.c
        src/code_generator.h src/code_generator_simple.c)

add_executable(ControlFlowTester
        src/symtable.h src/symtable.c
        src/control_flow.h src/control_flow.c
        src/code_generator.h src/code_generator_simple.c
        src/optimiser.h src/optimiser.c
        src/stderr_message.h src/stderr_message.c
        src/cftest.c)

# ------- Tests -------
add_executable(Test_mutable_string src/mutable_string.h src/mutable_string.c src/tests/mutable_string.cpp)
target_link_libraries(Test_mutable_string gtest gtest_main)

add_executable(Test_scanner
        src/scanner.h src/scanner_static.h src/scanner.c
        src/stderr_message.h
        src/mutable_string.h src/mutable_string.c
        src/tests/tests_common.h src/tests/stdin_mock_test.h
        src/tests/scanner.cpp)
target_link_libraries(Test_scanner gtest gtest_main)

add_executable(Test_parser_scanner
        src/scanner.h src/scanner_static.h src/scanner.c
        src/stderr_message.h
        src/mutable_string.h src/mutable_string.c
        src/parser.h src/parser.c
        src/tests/tests_common.h src/tests/stdin_mock_test.h
        src/tests/parser_scanner.cpp
        src/precedence_parser.h src/precedence_parser.c
        src/stacks.h src/stacks.c)
target_link_libraries(Test_parser_scanner gtest gtest_main)

add_executable(Test_symbol_table
        src/stderr_message.h
        src/tests/tests_common.h
        src/tests/symbol_table.cpp
        src/symtable.h src/symtable.c)
target_link_libraries(Test_symbol_table gtest gtest_main)

add_test(mutable_string Test_mutable_string)
add_test(scanner Test_scanner)
add_test(parser_scanner Test_parser_scanner)
add_test(symbol_table Test_symbol_table)

