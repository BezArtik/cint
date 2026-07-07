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
    interpreter-lib
)

include(GoogleTest)
gtest_discover_tests(tests)

if(ENABLE_SANITIZERS)
    set(SAN_FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_compile_options(tests PRIVATE ${SAN_FLAGS})
    target_link_options(tests PRIVATE ${SAN_FLAGS})
endif()

if(ENABLE_WARNINGS_AS_ERRORS)
    set(WARN_FLAGS -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(tests PRIVATE ${WARN_FLAGS})
endif()
