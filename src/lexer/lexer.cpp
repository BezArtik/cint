// lexer/lexer.cpp

#include "lexer/lexer.hpp"

#include "core/error/error_codes.hpp"
#include "core/error/error_report.hpp"
#include "core/token/keywords.hpp"
#include "core/token/token_types.hpp"

#include <cctype>
#include <string_view>

namespace lexer {

using tt = core::token_type;
using err = core::error_code;

lexer::lexer(std::string_view source, core::error_reporter& reporter) : source_(source), reporter_(reporter) {}

std::vector<core::token> lexer::scan_tokens() {
    while (!is_at_end()) {
        start_ = current_;
        scan_token();
    }

    tokens_.emplace_back(tt::END_OF_FILE, "", loc_);
    return tokens_;
}

void lexer::scan_token() {
    auto c = advance();
    switch (c) {
        case '(':
            add_token(tt::LEFT_PAREN);
            break;
        case ')':
            add_token(tt::RIGHT_PAREN);
            break;
        case '{':
            add_token(tt::LEFT_BRACE);
            break;
        case '}':
            add_token(tt::RIGHT_BRACE);
            break;
        case '[':
            add_token(tt::LEFT_BRACKET);
            break;
        case ']':
            add_token(tt::RIGHT_BRACKET);
            break;
        case ',':
            add_token(tt::COMMA);
            break;
        case '.':
            add_token(tt::DOT);
            break;
        case '+':
            add_token(match('=') ? tt::PLUS_EQUAL : match('+') ? tt::INCREMENT : tt::PLUS);
            break;
        case '-':
            add_token(match('=') ? tt::MINUS_EQUAL : match('-') ? tt::DECREMENT : tt::MINUS);
            break;
        case '*':
            add_token(match('=') ? tt::STAR_EQUAL : tt::STAR);
            break;
        case '%':
            add_token(match('=') ? tt::PERCENT_EQUAL : tt::PERCENT);
            break;
        case '^':
            add_token(match('=') ? tt::XOR_EQUAL : tt::XOR);
        case '~':
            add_token(match('=') ? tt::BIT_NOT_EQUAL : tt::BIT_NOT);
        case ';':
            add_token(tt::SEMICOLON);
            break;

        case '!':
            add_token(match('=') ? tt::BANG_EQUAL : tt::BANG);
            break;
        case '=':
            add_token(match('=') ? tt::EQUAL_EQUAL : tt::EQUAL);
            break;
        case '<':
            add_token(match('=') ? tt::LESS_EQUAL : match('<') ? tt::SHL : match('=') ? tt::SHL_EQUAL : tt::LESS);
            break;
        case '>':
            add_token(match('=') ? tt::GREATER_EQUAL : match('>') ? tt::SHR : match('=') ? tt::SHR_EQUAL : tt::GREATER);
            break;

        case '&':
            add_token(match('&') ? tt::LOGICAL_AND : match('=') ? tt::BIT_AND_EQUAL : tt::BIT_AND);
            break;
        case '|':
            add_token(match('|') ? tt::LOGICAL_OR : match('=') ? tt::BIT_OR_EQUAL : tt::BIT_OR);
            break;

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

        case '"':
            consume_string();
            break;

        default:
            if (std::isdigit(c)) {
                consume_number();
            } else if (std::isalpha(c) || c == '_') {
                consume_identifier();
            } else {
                reporter_.error(loc_, err::unexpected_character, c);
            }
            break;
    }
}

void lexer::consume_identifier() {
    while (std::isalnum(peek()) || peek() == '_') advance();

    auto lexeme = source_.substr(start_, current_ - start_);
    auto kw = core::lookup_keyword(lexeme);

    kw ? add_token(tt::KEYWORD) : add_token(tt::IDENTIFIER);
}

void lexer::consume_number() {
    bool is_double = false;

    while (std::isdigit(peek())) advance();

    if (peek() == '.' && std::isdigit(peek_next())) {
        is_double = true;
        advance();
        while (std::isdigit(peek())) advance();
    }

    auto text = source_.substr(start_, current_ - start_);
    tokens_.emplace_back(tt::NUMBER, text, loc_, is_double);
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

}  // namespace lexer
