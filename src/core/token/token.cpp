// core/token/token.cpp 

#include "core/token/token.hpp"
#include "core/token/keywords.hpp"
#include <optional>

namespace core {

token::token(token_type type, std::string_view lex, uint32_t line, uint32_t column)
    : type_(type), lexeme_(lex), line_(line), column_(column) {}

std::optional<keyword_info> token::as_keyword() const {
    if (!is_keyword()) return std::nullopt;
    return lookup_keyword(lexeme_);
}

bool token::is_keyword() const noexcept {
    return type_ == token_type::KEYWORD;
}

bool token::is_double_literal() const noexcept {
    return type_ == token_type::NUMBER &&
        lexeme_.find('.') != std::string_view::npos;
}

bool token::is_string_literal() const noexcept {
    return type_ == token_type::STRING;
}

bool token::is_identifier() const noexcept {
    return type_ == token_type::IDENTIFIER;
}

} // namespace core