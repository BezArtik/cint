add_library(interpreter STATIC)

target_compile_features(interpreter PUBLIC cxx_std_20)

target_sources(interpreter PRIVATE
    lexer/lexer.cpp
    parser/parser.cpp
    core/error/error_report.cpp
    core/token/keywords.cpp
    core/utils/builtins.cpp
    core/token/token_types.cpp
    core/token/token.cpp
    semantics/symbol_table.cpp
    semantics/type_check.cpp
    runtime/value.cpp
    runtime/environment.cpp
    runtime/interpreter.cpp
    debug/debug.cpp
)

target_include_directories(interpreter PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

add_executable(cint main.cpp)
target_link_libraries(cint PRIVATE interpreter)