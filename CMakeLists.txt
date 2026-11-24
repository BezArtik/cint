# CmakeLists.txt

cmake_minimum_required(VERSION 3.20...4.1)
project(interpreter LANGUAGES CXX)

option(BUILD_TESTS "Build tests" ON)
option(ENABLE_SANITIZERS "Enable ASan and UBSan" OFF)
option(ENABLE_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

include(FetchContent)

FetchContent_Declare(
    cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.7.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(cli11)

FetchContent_Declare(
    GoogleTest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(GoogleTest)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(AST_SPEC_FILE "${CMAKE_CURRENT_SOURCE_DIR}/include/ast/nodes.toml")
set(AST_GEN_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_ast.py")
set(AST_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

set(AST_GENERATED_HEADERS
    "${AST_GEN_DIR}/ast/nodes.hpp"
    "${AST_GEN_DIR}/ast/expression.hpp"
    "${AST_GEN_DIR}/ast/statement.hpp"
)

add_custom_command(
    OUTPUT ${AST_GENERATED_HEADERS}
    COMMAND "${Python3_EXECUTABLE}" "${AST_GEN_SCRIPT}"
            --input "${AST_SPEC_FILE}"
            --out-dir "${AST_GEN_DIR}"
    DEPENDS "${AST_SPEC_FILE}" "${AST_GEN_SCRIPT}"
    COMMENT "Generating AST headers from nodes.toml"
    VERBATIM
)

add_custom_target(generate_ast DEPENDS ${AST_GENERATED_HEADERS})

add_library(ast-headers INTERFACE)
add_dependencies(ast-headers generate_ast)
target_include_directories(ast-headers INTERFACE
    "$<BUILD_INTERFACE:${AST_GEN_DIR}>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
