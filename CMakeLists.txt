cmake_minimum_required(VERSION 3.20...4.1)
project(interpreter LANGUAGES CXX)

option(BUILD_TESTS "Build tests" ON)
option(ENABLE_SANITIZERS "Enable ASan and UBSan" OFF)
option(ENABLE_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
