// core/token/token_types.hpp

#pragma once
#include <array>
#include <cstdint>
#include <string_view>

namespace core {

#define TOKEN_TYPES(X) \
    X(LEFT_PAREN)      \
    X(RIGHT_PAREN)     \
    X(LEFT_BRACE)      \
    X(RIGHT_BRACE)     \
    X(LEFT_BRACKET)    \
    X(RIGHT_BRACKET)   \
    X(COMMA)           \
    X(DOT)             \
    X(SEMICOLON)       \
    X(PLUS)            \
    X(MINUS)           \
    X(STAR)            \
    X(SLASH)           \
    X(PERCENT)         \
    X(BANG)            \
    X(EQUAL)           \
    X(BANG_EQUAL)      \
    X(EQUAL_EQUAL)     \
    X(GREATER)         \
    X(GREATER_EQUAL)   \
    X(LESS)            \
    X(LESS_EQUAL)      \
    X(INCREMENT)       \
    X(DECREMENT)       \
    X(PLUS_EQUAL)      \
    X(MINUS_EQUAL)     \
    X(STAR_EQUAL)      \
    X(SLASH_EQUAL)     \
    X(PERCENT_EQUAL)   \
    X(BIT_AND)         \
    X(BIT_OR)          \
    X(XOR)             \
    X(BIT_NOT)         \
    X(SHL)             \
    X(SHR)             \
    X(BIT_AND_EQUAL)   \
    X(BIT_OR_EQUAL)    \
    X(XOR_EQUAL)       \
    X(BIT_NOT_EQUAL)   \
    X(SHL_EQUAL)       \
    X(SHR_EQUAL)       \
    X(LOGICAL_AND)     \
    X(LOGICAL_OR)      \
    X(IDENTIFIER)      \
    X(STRING)          \
    X(NUMBER)          \
    X(KW_IF)           \
    X(KW_ELSE)         \
    X(KW_WHILE)        \
    X(KW_FOR)          \
    X(KW_RETURN)       \
    X(KW_TRUE)         \
    X(KW_FALSE)        \
    X(KW_INT)          \
    X(KW_DOUBLE)       \
    X(KW_BOOL)         \
    X(KW_STRING)       \
    X(KW_STRUCT)       \
    X(KW_VOID)         \
    X(END_OF_FILE)     \
    X(UNKNOWN)

enum class token_type : uint8_t {
#define X(name) name,
    TOKEN_TYPES(X)
#undef X
};

inline constexpr std::array token_type_names = {
#define X(name) std::string_view(#name),
    TOKEN_TYPES(X)
#undef X
};

}  // namespace core
