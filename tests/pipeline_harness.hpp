#pragma once

#include "ast/statement.hpp"
#include "core/error/error_report.hpp"
#include "core/utils/arena.hpp"
#include "core/utils/symbol_registry.hpp"
#include "lexer/lexer.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace tests {

class pipeline_harness {
public:
    pipeline_harness(std::string_view source);

    bool lex();
    bool parse();
    bool check_semantics();
    bool interpret();
    bool run_all();

    const lexer::token_list& tokens() const noexcept;
    const ast::stmt_list& ast() const noexcept;
    bool had_error() const noexcept;

private:
    std::string source_code_;
    core::error_reporter reporter_;
    core::arena arena_;
    core::arena_memory_resource mr_;
    lexer::token_list tokens_;
    ast::stmt_list ast_;
    std::optional<core::symbol_registry> registry_;
};

}  // namespace tests
