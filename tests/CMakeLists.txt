cmake_minimum_required(VERSION 3.20)

include(FetchContent)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

set(INSTALL_GTEST OFF CACHE BOOL "Disable install" FORCE)

FetchContent_Declare(
    GoogleTest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(GoogleTest)

add_executable(tests)

target_sources(tests PRIVATE
    test_interpreter.cpp
    test_lexer.cpp
    test_parser.cpp
    test_semantics.cpp
    test_value.cpp
)

target_link_libraries(tests PRIVATE
    GTest::gtest_main
    interpreter
)

target_include_directories(tests PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

include(GoogleTest)
gtest_discover_tests(tests)