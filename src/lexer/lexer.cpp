// lexer/lexer.cpp


#include "lexer/lexer.hpp"
#include "core/token/token_types.hpp"
#include "core/token/keywords.hpp"
#include "core/error/error_report.hpp"
#include "core/error/error_codes.hpp"
#include <string_view>
#include <unordered_map>
#include <cctype>

namespace lexer {

lexer::lexer(std::string_view source, core::error_reporter& reporter)
    : source_(source), reporter_(reporter) {
}

std::vector<core::token> lexer::scan_tokens() {
    while (!is_at_end()) {
        start_ = current_;
        scan_token();
    }

    tokens_.emplace_back(core::token_type::END_OF_FILE, "", line_, column_);
    return tokens_;
}

void lexer::scan_token() {
    auto c = advance();
    switch (c) {
    case '(': add_token(core::token_type::LEFT_PAREN); break;
    case ')': add_token(core::token_type::RIGHT_PAREN); break;
    case '{': add_token(core::token_type::LEFT_BRACE); break;
    case '}': add_token(core::token_type::RIGHT_BRACE); break;
	case '[': add_token(core::token_type::LEFT_BRACKET); break;
	case ']': add_token(core::token_type::RIGHT_BRACKET); break;
    case ',': add_token(core::token_type::COMMA); break;
    case '.': add_token(core::token_type::DOT); break;
    case '+': add_token(match('=') ? core::token_type::PLUS_EQUAL 
        : match('+') ? core::token_type::INCREMENT : core::token_type::PLUS); break;
    case '-': add_token(match('=') ? core::token_type::MINUS_EQUAL 
        : match('-') ? core::token_type::DECREMENT : core::token_type::MINUS); break;
    case '*': add_token(match('=') ? core::token_type::STAR_EQUAL : core::token_type::STAR); break;
    case '%': add_token(match('=') ? core::token_type::PERCENT_EQUAL : core::token_type::PERCENT); break;
	case ';': add_token(core::token_type::SEMICOLON); break;

    case '!': add_token(match('=') ? core::token_type::BANG_EQUAL : core::token_type::BANG); break;
    case '=': add_token(match('=') ? core::token_type::EQUAL_EQUAL : core::token_type::EQUAL); break;
    case '<': add_token(match('=') ? core::token_type::LESS_EQUAL : core::token_type::LESS); break;
    case '>': add_token(match('=') ? core::token_type::GREATER_EQUAL : core::token_type::GREATER); break;
        
    case '&': add_token(match('&') ? core::token_type::AND : core::token_type::UNKNOWN); break;
    case '|': add_token(match('|') ? core::token_type::OR : core::token_type::UNKNOWN); break;
	
    case '/':
		if (match('/')) {
			while (peek() != '\n' && !is_at_end()) advance();
		} else {
			add_token(match('=') ? core::token_type::SLASH_EQUAL : core::token_type::SLASH);
		}
		break;

    case ' ':
    case '\r':
    case '\t': break;

    case '\n':
        line_++;
        column_ = 0;
        break;

    case '"': consume_string(); break;

    default:
        if (std::isdigit(c)) {
            consume_number();
        } else if (std::isalpha(c) || c == '_') {
            consume_identifier();
        } else {
			reporter_.error(line_, column_, core::error_code::unexpected_character, c);
        }
        break;
    }
}

void lexer::consume_identifier() {
    while (std::isalnum(peek()) || peek() == '_') advance();

    auto lexeme = source_.substr(start_, current_ - start_);
    auto kw = core::lookup_keyword(lexeme);

    kw ? add_token(core::token_type::KEYWORD) 
       : add_token(core::token_type::IDENTIFIER);
}

void lexer::consume_number() {
    while (std::isdigit(peek())) advance();

    if (peek() == '.' && std::isdigit(peek_next())) {
        advance();
        while (std::isdigit(peek())) advance();
    }

    add_token(core::token_type::NUMBER);
}

void lexer::consume_string() {
    while (peek() != '"' && !is_at_end()) {
        if (peek() == '\n') {
            line_++;
            column_ = 1;
        }
        advance();
    }

    if (is_at_end()) {
        reporter_.error(line_, column_, core::error_code::unterminated_string);
        return;
    }

    advance(); 
    add_token(core::token_type::STRING);
}

char lexer::advance() noexcept {
    column_++;
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
    column_++;
    return true;
}

bool lexer::is_at_end() const noexcept {
    return current_ >= source_.length();
}

void lexer::add_token(core::token_type type) {
    auto text = source_.substr(start_, current_ - start_);
    tokens_.emplace_back(type, text, line_, column_);
}


} // namespace lexer