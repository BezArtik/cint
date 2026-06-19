cmake_minimum_required(VERSION 3.20...4.1)
project(interpreter)

option(BUILD_TESTS "Build tests" ON)

add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
