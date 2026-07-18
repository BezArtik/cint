add_library(interpreter-lib STATIC)

target_compile_features(interpreter-lib PRIVATE cxx_std_20)

target_sources(interpreter-lib PRIVATE
    lexer/lexer.cpp
    parser/parser.cpp
    core/error/error_report.cpp
    core/utils/builtins.cpp
    core/utils/function_registry.cpp
    core/token/type.cpp
    core/value/value.cpp
    core/value/operations.cpp
    semantics/type_check.cpp
    runtime/interpreter.cpp
    debug/debug.cpp
)

target_include_directories(interpreter-lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)


add_executable(cint main.cpp)
target_link_libraries(cint PRIVATE interpreter-lib)


if(ENABLE_SANITIZERS)
    set(SAN_FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer)

    target_compile_options(interpreter-lib PRIVATE ${SAN_FLAGS})
    target_link_options(interpreter-lib PRIVATE ${SAN_FLAGS})

    target_compile_options(cint PRIVATE ${SAN_FLAGS})
    target_link_options(cint PRIVATE ${SAN_FLAGS})
endif()


if(ENABLE_WARNINGS_AS_ERRORS)
    set(WARN_FLAGS -Wall -Wextra -Wpedantic -Werror)

    target_compile_options(interpreter-lib PRIVATE ${WARN_FLAGS})
    target_compile_options(cint PRIVATE ${WARN_FLAGS})
endif()
