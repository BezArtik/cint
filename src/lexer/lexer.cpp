// lexer/lexer.cpp

#include "lexer/lexer.hpp"

#include "core/error/error_codes.hpp"
#include "core/error/error_report.hpp"
#include "core/token/keywords.hpp"
#include "core/token/token_types.hpp"
#include "core/utils/arena.hpp"
#include "core/value/value.hpp"

#include <cctype>
#include <string_view>

using tt = core::token_type;
using err = core::error_code;

lexer::lexer(std::string_view source, core::error_reporter& reporter, core::arena_memory_resource& mr)
    : source_(source), reporter_(reporter), mr_(mr), tokens_(&mr_) {}

lexer::token_list lexer::scan_tokens() {
    while (!is_at_end()) {
        start_ = current_;
        scan_token();
    }

    tokens_.emplace_back(tt::END_OF_FILE, "", loc_);
    return std::move(tokens_);
}

// clang-format off
void lexer::scan_token() {
    auto c = advance();
    switch (c) {
        case '(': add_token(tt::LEFT_PAREN); break;
        case ')': add_token(tt::RIGHT_PAREN); break;
        case '{': add_token(tt::LEFT_BRACE); break;
        case '}': add_token(tt::RIGHT_BRACE); break;
        case '[': add_token(tt::LEFT_BRACKET); break;
        case ']': add_token(tt::RIGHT_BRACKET); break;
        case ',': add_token(tt::COMMA); break;
        case '.': add_token(tt::DOT); break;
        case '+': add_token(match('=') ? tt::PLUS_EQUAL : match('+') ? tt::INCREMENT : tt::PLUS); break;
        case '-': add_token(match('=') ? tt::MINUS_EQUAL : match('-') ? tt::DECREMENT : tt::MINUS); break;
        case '*': add_token(match('=') ? tt::STAR_EQUAL : tt::STAR); break;
        case '%': add_token(match('=') ? tt::PERCENT_EQUAL : tt::PERCENT); break;
        case '^': add_token(match('=') ? tt::XOR_EQUAL : tt::XOR); break;
        case '~': add_token(match('=') ? tt::BIT_NOT_EQUAL : tt::BIT_NOT); break;
        case ';': add_token(tt::SEMICOLON); break;

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
            loc_.column_ = 0;
            break;

        case '"': consume_string(); break;

        default:
            if (std::isdigit(c)) {
                consume_number();
            } else if (std::isalpha(c) || c == '_') {
                consume_identifier_or_keyword();
            } else {
                reporter_.error(loc_, err::unexpected_character, c);
            }
            break;
    }
}
// clang-format on

void lexer::consume_identifier_or_keyword() {
    while (std::isalnum(peek()) || peek() == '_') advance();

    auto lexeme = source_.substr(start_, current_ - start_);
    auto type = core::lookup_keyword(lexeme);

    std::optional<core::value> literal_val;
    if (type == tt::KW_TRUE)
        literal_val = true;
    else if (type == tt::KW_FALSE)
        literal_val = false;

    tokens_.emplace_back(type, lexeme, loc_, literal_val);
}

void lexer::consume_number() {
    while (std::isdigit(peek())) advance();

    bool is_double = false;
    if (peek() == '.' && std::isdigit(peek_next())) {
        is_double = true;
        advance();
        while (std::isdigit(peek())) advance();
    }

    auto text = source_.substr(start_, current_ - start_);

    try {
        auto val = core::value::from_string(text, is_double);
        tokens_.emplace_back(tt::NUMBER, text, loc_, val);
    } catch (const core::interpret_error&) { reporter_.error(loc_, err::unexpected_character); }
}

void lexer::consume_string() {
    while (peek() != '"' && !is_at_end()) {
        if (peek() == '\n') {
            loc_.line_++;
            loc_.column_ = 1;
        }
        advance();
    }

    if (is_at_end()) {
        reporter_.error(loc_, err::unterminated_string);
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

void lexer::add_token(tt type) {
    auto text = source_.substr(start_, current_ - start_);
    tokens_.emplace_back(type, text, loc_);
}
