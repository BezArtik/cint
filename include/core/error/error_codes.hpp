// core/error/error_codes.hpp
#pragma once
#include <array>
#include <cstdint>
#include <string_view>

namespace cint::core {
// clang-format off
#define ERROR(X)                                                                                \
    X(unexpected_character,                  "Unexpected character '{}'")                       \
    X(unterminated_string,                   "Unterminated string literal")                     \
                                                                                                \
    X(unexpected_token,                      "Unexpected token")                                \
    X(expected_expression,                   "Expected expression")                             \
    X(expected_semicolon,                    "Expected ';'")                                    \
    X(expected_identifier,                   "Expected identifier")                             \
    X(expected_right_paren,                  "Expected ')'")                                    \
    X(expected_right_brace,                  "Expected '}'")                                    \
    X(expected_left_brace,                   "Expected '{'")                                    \
    X(expected_right_bracket,                "Expected ']'")                                    \
    X(expected_left_paren,                   "Expected '('")                                    \
    X(expected_equal,                        "Expected '='")                                    \
    X(expected_left_paren_for,               "Expected '(' after 'for'")                        \
    X(nested_declaration,                    "Nested fn/struct declarations are not supported") \
    X(invalid_number,                        "Invalid number")                                  \
                                                                                                \
    X(redeclaration,                         "Redeclaration of '{}'")                           \
                                                                                                \
    X(undefined_variable,                    "Undefined variable '{}'")                         \
    X(undefined_function,                    "Undefined function '{}'")                         \
    X(not_an_lvalue,                         "Expression is not an lvalue")                     \
                                                                                                \
    X(argument_count_mismatch,               "Function '{}' expects {} arguments, got {}")      \
    X(stack_overflow,                        "Stack overflow")                                  \
                                                                                                \
    X(return_outside_function,               "Return statement outside of function")            \
                                                                                                \
    X(indexing_non_array,                    "Indexing operator applied to non-array type")     \
    X(index_must_be_integer,                 "Array index must be of integer type")             \
    X(index_out_of_bounds,                   "Index out of bounds")                             \
    X(not_a_struct,                          "Left side of '.' is not a structure")             \
    X(no_such_field,                         "Structure '{}' has no field '{}'")                \
    X(no_such_method,                        "Type '{}' has no method '{}'")                    \
    X(this_outside_method,                   "'this' is only available inside a method")        \
                                                                                                \
    X(unsupported_binary_operator,           "Unsupported binary operator")                     \
    X(unsupported_unary_operator,            "Unsupported unary operator")                      \
    X(unsupported_postfix_operator,          "Unsupported postfix operator")                    \
                                                                                                \
    X(division_by_zero,                      "Division by zero")                                \
    X(invalid_conversion,                    "Invalid type conversion")
// clang-format on

enum class error_code : std::uint8_t {
#define X(code, msg) code,
  ERROR(X)
#undef X
};

inline constexpr std::array error_table = {
#define X(code, msg) std::pair{error_code::code, std::string_view{msg}},
    ERROR(X)
#undef X
};

#undef ERROR

}  // namespace cint::core
