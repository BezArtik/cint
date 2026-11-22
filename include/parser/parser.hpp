// parser/parser.hpp
#pragma once
#include "ast/node.hpp"
#include "core/error/diagnostic.hpp"
#include "core/error/error_codes.hpp"
#include "core/memory/arena.hpp"
#include "core/token/token.hpp"
#include "core/utils/literal_pool.hpp"
#include "parser/slot_allocator.hpp"

#include <span>

namespace cint {

class parser {
public:
    parser(std::span<const core::token> tokens, core::diagnostic_sink& sink, core::arena& arena,
           core::literal_pool& literals)
        : tokens_{tokens}, sink_{sink}, arena_{arena}, literals_{literals} {}

    [[nodiscard]] ast::stmt_list parse();

private:
    const core::token& advance() noexcept;
    const core::token& peek() const noexcept;
    const core::token& prev() const noexcept;
    bool is_at_end() const noexcept;
    bool check(core::token_type type) const noexcept;
    bool match(std::initializer_list<core::token_type> types) noexcept;
    const core::token& consume(core::token_type type, core::error_code code);

    std::optional<ast::statement> declaration();
    ast::statement statement();
    ast::statement let_declaration();
    ast::statement fn_declaration();
    ast::statement struct_declaration();
    ast::statement while_statement();
    ast::statement for_statement();
    ast::statement if_statement();
    ast::statement block_statement();
    ast::statement return_statement();

    ast::expression parse_expression(std::int8_t precedence);
    ast::expression assignment();
    ast::expression unary();
    ast::expression postfix();
    ast::expression primary();

    ast::expr_list parse_call_args();

    void synchronize();

    std::span<const core::token> tokens_;
    core::diagnostic_sink& sink_;
    std::size_t current_{};
    core::arena& arena_;
    core::literal_pool& literals_;
    slot_allocator slots_;
    std::uint32_t global_max_slots_{};
};

}  // namespace cint
