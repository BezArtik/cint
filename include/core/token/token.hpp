// core/token/token.hpp

#pragma once

#include "core/value/value.hpp"

#include <cstdint>
#include <optional>
#include <string_view>

namespace core {

enum class token_type : uint8_t;

struct location {
    uint32_t line_;
    uint32_t column_;
};

struct token {
    token_type type_;
    std::string_view lexeme_;
    location loc_;
    std::optional<value> literal_value_;

    token() = default;
    token(token_type type, std::string_view lex, location loc, std::optional<value> val = std::nullopt)
        : type_(type), lexeme_(lex), loc_(loc), literal_value_(val) {}
};

}  // namespace core
