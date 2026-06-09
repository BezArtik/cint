// lexer/lexer.hpp


#pragma once
#include "core/token/token.hpp"
#include "core/error/error_report.hpp"
#include <vector>
#include <string_view>

namespace lexer {

class lexer {
public:

	lexer(std::string_view src, core::error_reporter& reporter);

	std::vector<core::token> scan_tokens();

private:

	bool is_at_end() const noexcept;
	char advance() noexcept;
	char peek() const noexcept;
    char peek_next() const noexcept;
	bool match(char expected) noexcept;

	void scan_token();
    void add_token(core::token_type type);

    void consume_string();
    void consume_number();
    void consume_identifier();

    std::string_view source_;
    core::error_reporter& reporter_;
    std::vector<core::token> tokens_;

    size_t start_ = 0;
    size_t current_ = 0;
    core::location loc_{ 1,1 };
};

} // namespace lexer