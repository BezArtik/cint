include(FetchContent)

FetchContent_Declare(
    GoogleTest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(GoogleTest)

add_executable(tests
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

include(GoogleTest)
gtest_discover_tests(tests)
