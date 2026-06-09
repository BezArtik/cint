// core/token/token.hpp


#pragma once
#include "core/token/token_types.hpp"
#include <string_view>
#include <optional>
#include <cstdint>


namespace core {

struct keyword_info;
enum class token_type : uint8_t;

struct location {
    uint32_t line_{};
    uint32_t column_{};
};

struct token {
    token_type type_{};
    std::string_view lexeme_;
    location loc_{};

    token() = default;
    token(token_type type, std::string_view lex, location loc);

    bool is_keyword() const noexcept;
    bool is_double_literal() const noexcept;
    bool is_string_literal() const noexcept;
    bool is_identifier() const noexcept;

    std::optional<keyword_info> as_keyword() const;
};

} // namespace core