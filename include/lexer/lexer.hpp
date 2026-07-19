// lexer/lexer.hpp

#pragma once
#include "core/error/error_report.hpp"
#include "core/token/token.hpp"
#include "core/utils/arena.hpp"

#include <string_view>
#include <vector>

class lexer {
public:
    using token_list = std::pmr::vector<core::token>;

    lexer(std::string_view src, core::error_reporter& reporter, core::arena_memory_resource& mr);

    token_list scan_tokens();

private:
    bool is_at_end() const noexcept;
    char advance() noexcept;
    char peek() const noexcept;
    char peek_next() const noexcept;
    bool match(char expected) noexcept;

    void scan_token();
    void add_token(core::token_type type);
    void add_token(core::token_type type, core::location loc);

    void consume_string();
    std::string process_escape_sequences(std::string_view raw, core::location start_loc);
    void consume_number(core::location start_loc);
    void consume_identifier_or_keyword(core::location start_loc);

    std::string_view source_;
    core::error_reporter& reporter_;
    core::arena_memory_resource& mr_;
    token_list tokens_;

    size_t start_ = 0;
    size_t current_ = 0;
    core::location loc_{1, 1};
};
