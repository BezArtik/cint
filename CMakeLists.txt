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

add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
