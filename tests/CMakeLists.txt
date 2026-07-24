include(FetchContent)

FetchContent_Declare(
    GoogleTest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(GoogleTest)

add_library(test_harness STATIC
    pipeline_harness.hpp
    pipeline_harness.cpp
)
target_link_libraries(test_harness PRIVATE interpreter-lib)
target_include_directories(test_harness PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src/../include
)

add_executable(tests
    test_interpreter.cpp
    test_lexer.cpp
    test_parser.cpp
    test_semantics.cpp
    test_value.cpp
)

target_link_libraries(tests PRIVATE
    GTest::gtest_main
    test_harness
)

target_include_directories(tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src/../include
)

include(GoogleTest)
gtest_discover_tests(tests)

if(ENABLE_SANITIZERS)
    set(SAN_FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_compile_options(tests PRIVATE ${SAN_FLAGS})
    target_compile_options(test_harness PRIVATE ${SAN_FLAGS})
    target_link_options(tests PRIVATE ${SAN_FLAGS})
endif()

if(ENABLE_WARNINGS_AS_ERRORS)
    set(WARN_FLAGS -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(tests PRIVATE ${WARN_FLAGS})
    target_compile_options(test_harness PRIVATE ${WARN_FLAGS})
endif()
