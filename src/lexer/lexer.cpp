// lexer/lexer.cpp

#include "lexer/lexer.hpp"

#include "core/error/error_codes.hpp"
#include "core/error/error_report.hpp"
#include "core/token/keywords.hpp"
#include "core/token/token.hpp"
#include "core/token/token_types.hpp"
#include "core/utils/arena.hpp"
#include "core/value/value.hpp"

#include <cctype>
#include <string_view>

using tt = core::token_type;
using err = core::error_code;

lexer::lexer(std::string_view source, core::error_reporter& reporter, core::arena_memory_resource& mr)
    : source_{source}, reporter_{reporter}, mr_{mr}, tokens_{&mr_} {}

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
    add_token(type, loc_);
}

void lexer::add_token(tt type, core::location loc) {
    auto&& text = source_.substr(start_, current_ - start_);
    tokens_.emplace_back(type, text, loc);
}

// clang-format off
void lexer::scan_token() {
    auto&& start_loc = loc_; 
    auto&& c = advance();
    switch (c) {
        case '(': add_token(tt::LEFT_PAREN, start_loc); break;
        case ')': add_token(tt::RIGHT_PAREN, start_loc); break;
        case '{': add_token(tt::LEFT_BRACE, start_loc); break;
        case '}': add_token(tt::RIGHT_BRACE, start_loc); break;
        case '[': add_token(tt::LEFT_BRACKET, start_loc); break;
        case ']': add_token(tt::RIGHT_BRACKET, start_loc); break;
        case ',': add_token(tt::COMMA, start_loc); break;
        case '.': add_token(tt::DOT, start_loc); break;
        case ';': add_token(tt::SEMICOLON, start_loc); break;

        case '+': add_token(match('=') ? tt::PLUS_EQUAL : match('+') ? tt::INCREMENT : tt::PLUS, start_loc); break;
        case '-': add_token(match('=') ? tt::MINUS_EQUAL : match('-') ? tt::DECREMENT : tt::MINUS, start_loc); break;
        case '*': add_token(match('=') ? tt::STAR_EQUAL : tt::STAR, start_loc); break;
        case '%': add_token(match('=') ? tt::PERCENT_EQUAL : tt::PERCENT, start_loc); break;
        case '^': add_token(match('=') ? tt::XOR_EQUAL : tt::XOR, start_loc); break;
        case '~': add_token(match('=') ? tt::BIT_NOT_EQUAL : tt::BIT_NOT, start_loc); break;

        case '!': add_token(match('=') ? tt::BANG_EQUAL : tt::BANG, start_loc); break;
        case '=': add_token(match('=') ? tt::EQUAL_EQUAL : tt::EQUAL, start_loc); break;

        case '<':
            if (match('='))
                add_token(tt::LESS_EQUAL, start_loc);
            else if (match('<'))
                add_token(match('=') ? tt::SHL_EQUAL : tt::SHL, start_loc);
            else
                add_token(tt::LESS, start_loc);
            break;

        case '>':
            if (match('='))
                add_token(tt::GREATER_EQUAL, start_loc);
            else if (match('>'))
                add_token(match('=') ? tt::SHR_EQUAL : tt::SHR, start_loc);
            else
                add_token(tt::GREATER, start_loc);
            break;

        case '&': add_token(match('&') ? tt::LOGICAL_AND : match('=') ? tt::BIT_AND_EQUAL : tt::BIT_AND, start_loc); break;
        case '|': add_token(match('|') ? tt::LOGICAL_OR : match('=') ? tt::BIT_OR_EQUAL : tt::BIT_OR, start_loc); break;

        case '/':
            if (match('/')) {
                while (peek() != '\n' && !is_at_end()) advance();
            } else {
                add_token(match('=') ? tt::SLASH_EQUAL : tt::SLASH, start_loc);
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

        case '"': consume_string(start_loc); break;
        
        default:
            if (std::isdigit(c)) {
                consume_number(start_loc);
            } else if (std::isalpha(c) || c == '_') {
                consume_identifier_or_keyword(start_loc);
            } else {
                reporter_.error(start_loc, err::unexpected_character, c);
            }
            break;
    }
}
// clang-format on
void lexer::consume_identifier_or_keyword(core::location start_loc) {
    while (std::isalnum(peek()) || peek() == '_') advance();

    auto&& lexeme = source_.substr(start_, current_ - start_);
    auto&& type = core::lookup_keyword(lexeme);

    std::optional<core::value> literal_val;
    if (type == tt::KW_TRUE)
        literal_val = true;
    else if (type == tt::KW_FALSE)
        literal_val = false;

    auto&& text = source_.substr(start_, current_ - start_);
    tokens_.emplace_back(type, text, start_loc, literal_val);
}

void lexer::consume_number(core::location start_loc) {
    while (std::isdigit(peek())) advance();

    bool is_double = false;
    if (peek() == '.' && std::isdigit(peek_next())) {
        is_double = true;
        advance();
        while (std::isdigit(peek())) advance();
    }

    auto&& text = source_.substr(start_, current_ - start_);

    try {
        auto&& val = core::value::from_string(text, is_double);
        tokens_.emplace_back(tt::NUMBER, text, start_loc, val);
    } catch (const core::value_error&) {
        reporter_.error(start_loc, err::unexpected_character);
        tokens_.emplace_back(tt::NUMBER, text, start_loc, std::nullopt);
    }
}

void lexer::consume_string(core::location start_loc) {
    while (peek() != '"' && !is_at_end()) {
        if (peek() == '\n') {
            loc_.line_++;
            loc_.column_ = 1;
        }
        if (peek() == '\\' && peek_next() != '\0') advance();
        advance();
    }

    if (is_at_end()) {
        reporter_.error(start_loc, err::unterminated_string);
        return;
    }

    advance();

    auto&& raw = source_.substr(start_ + 1, current_ - start_ - 2);
    auto&& processed = process_escape_sequences(raw, start_loc);

    auto&& text = source_.substr(start_, current_ - start_);
    tokens_.emplace_back(tt::STRING, text, start_loc, std::move(processed));
}
// clang-format off
std::string lexer::process_escape_sequences(std::string_view raw, core::location start_loc) {
    std::string result;
    result.reserve(raw.size());

    auto&& it = raw.begin();
    auto&& end = raw.end();

    while (it != end) {
        if (*it == '\\') {
            auto&& next = std::next(it);
            if (next == end) {
                reporter_.error(start_loc, err::unterminated_string);
                break;
            }
            switch (*next) {
                case 'n':  result.push_back('\n'); break;
                case 't':  result.push_back('\t'); break;
                case 'r':  result.push_back('\r'); break;
                case '\\': result.push_back('\\'); break;
                case '"':  result.push_back('"');  break;
                case '0':  result.push_back('\0'); break;
                default:
                    reporter_.error(start_loc, err::unexpected_character, *next);
                    result.push_back(*next);
                    break;
            }
            std::advance(it, 2);
        } else {
            result.push_back(*it);
            ++it;
        }
    }

    return result;
}
// clang-format on 

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
