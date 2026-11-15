// lexer/lexer.cpp

#include "lexer/lexer.hpp"

#include "core/error/error_codes.hpp"
#include "core/token/token.hpp"
#include "core/token/token_types.hpp"

#include <string_view>

namespace cint {

using tt = core::token_type;
using err = core::error_code;

namespace {

struct keyword_info {
    std::string_view lexeme_;
    tt type_;
};
// clang-format off
constexpr std::array keywords{
    keyword_info{"if", tt::KW_IF},         
    keyword_info{"else", tt::KW_ELSE},     
    keyword_info{"while", tt::KW_WHILE},
    keyword_info{"for", tt::KW_FOR},       
    keyword_info{"return", tt::KW_RETURN},

    keyword_info{"let", tt::KW_LET},       
    keyword_info{"fn", tt::KW_FN},         
    keyword_info{"struct", tt::KW_STRUCT},

    keyword_info{"true", tt::KW_TRUE},     
    keyword_info{"false", tt::KW_FALSE},
};
// clang-format on

bool is_digit(char c) noexcept {
    return c >= '0' && c <= '9';
}
bool is_alpha(char c) noexcept {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
bool is_alnum(char c) noexcept {
    return is_alpha(c) || is_digit(c);
}

}  // namespace

lexer::token_list lexer::scan_tokens() {
    start_ = 0;
    current_ = 0;
    loc_ = {1, 1};
    tokens_.clear();
    while (!is_at_end()) {
        start_ = current_;
        scan_token();
    }

    tokens_.emplace_back(tt::END_OF_FILE, "", loc_);
    return tokens_;
}

void lexer::add_token(tt type) {
    tokens_.emplace_back(type, source_.substr(start_, current_ - start_), start_loc_);
}

// clang-format off
void lexer::scan_token() {
    start_loc_ = loc_; 
    const auto c = advance();
    switch (c) {
        case '(': add_token(tt::LEFT_PAREN); break;
        case ')': add_token(tt::RIGHT_PAREN); break;
        case '{': add_token(tt::LEFT_BRACE); break;
        case '}': add_token(tt::RIGHT_BRACE); break;
        case '[': add_token(tt::LEFT_BRACKET); break;
        case ']': add_token(tt::RIGHT_BRACKET); break;
        case ',': add_token(tt::COMMA); break;
        case '.': add_token(tt::DOT); break;
        case ';': add_token(tt::SEMICOLON); break;

        case '+': add_token(match('=') ? tt::PLUS_EQUAL : match('+') ? tt::INCREMENT : tt::PLUS); break;
        case '-': add_token(match('=') ? tt::MINUS_EQUAL : match('-') ? tt::DECREMENT : tt::MINUS); break;
        case '*': add_token(match('=') ? tt::STAR_EQUAL : tt::STAR); break;
        case '%': add_token(match('=') ? tt::PERCENT_EQUAL : tt::PERCENT); break;
        case '^': add_token(match('=') ? tt::XOR_EQUAL : tt::XOR); break;

        case '!': add_token(match('=') ? tt::BANG_EQUAL : tt::BANG); break;
        case '=': add_token(match('=') ? tt::EQUAL_EQUAL : tt::EQUAL); break;

        case '<':
            if (match('='))
                add_token(tt::LESS_EQUAL);
            else if (match('<'))
                add_token(match('=') ? tt::SHL_EQUAL : tt::SHL);
            else
                add_token(tt::LESS);
            break;

        case '>':
            if (match('='))
                add_token(tt::GREATER_EQUAL);
            else if (match('>'))
                add_token(match('=') ? tt::SHR_EQUAL : tt::SHR);
            else
                add_token(tt::GREATER);
            break;

        case '&': add_token(match('&') ? tt::LOGICAL_AND : match('=') ? tt::BIT_AND_EQUAL : tt::BIT_AND); break;
        case '|': add_token(match('|') ? tt::LOGICAL_OR : match('=') ? tt::BIT_OR_EQUAL : tt::BIT_OR); break;

        case '/':
            if (match('/')) {
                while (peek() != '\n' && !is_at_end()) advance();
            } else {
                add_token(match('=') ? tt::SLASH_EQUAL : tt::SLASH);
            }
            break;

        case ' ':
        case '\r':
        case '\t':
            break;

        case '\n':
            loc_.line_++;
            loc_.column_ = 1;
            break;

        case '"': consume_string(); break;
        
        default:
            if (is_digit(c)) {
                consume_number();
            } else if (is_alpha(c) || c == '_') {
                consume_identifier_or_keyword();
            } else {
                sink_.report(start_loc_, err::unexpected_character, c);
            }
            break;
    }
}
// clang-format on
void lexer::consume_identifier_or_keyword() {
    while (is_alnum(peek()) || peek() == '_') advance();

    const auto it = std::ranges::find(keywords, source_.substr(start_, current_ - start_), &keyword_info::lexeme_);
    add_token(it != keywords.end() ? it->type_ : tt::IDENTIFIER);
}

void lexer::consume_number() {
    while (is_digit(peek())) advance();

    if (peek() == '.' && is_digit(peek_next())) {
        advance();
        while (is_digit(peek())) advance();
    }

    add_token(tt::NUMBER);
}

void lexer::consume_string() {
    while (peek() != '"' && !is_at_end()) {
        if (peek() == '\n') {
            loc_.line_++;
            loc_.column_ = 0;
        }
        if (peek() == '\\' && peek_next() != '\0') advance();
        advance();
    }

    if (is_at_end()) {
        sink_.report(start_loc_, err::unterminated_string);
        return;
    }

    advance();
    add_token(tt::STRING);
}

char lexer::advance() noexcept {
    loc_.column_++;
    return source_[current_++];
}

char lexer::peek() const noexcept {
    if (is_at_end()) return '\0';
    return source_[current_];
}

char lexer::peek_next() const noexcept {
    if (current_ + 1 >= source_.length()) return '\0';
    return source_[current_ + 1];
}

bool lexer::match(char expected) noexcept {
    if (is_at_end() || source_[current_] != expected) return false;
    current_++;
    loc_.column_++;
    return true;
}

bool lexer::is_at_end() const noexcept {
    return current_ >= source_.length();
}

}  // namespace cint
