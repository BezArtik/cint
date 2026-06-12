// parser/parser.hpp


#pragma once
#include "ast/statement.hpp"
#include "ast/expression.hpp"
#include "core/token/token.hpp"
#include "core/error/error_report.hpp"
#include <vector>
#include <memory>
#include <string_view>
#include <functional>

namespace parser {

class parser {
public:

    parser(const std::vector<core::token>& tokens, core::error_reporter& reporter);

    std::vector<ast::stmt_ptr> parse();

private:
    const core::token& advance() noexcept;
    const core::token& peek() const noexcept;
    const core::token& prev() const noexcept;
    bool is_at_end() const noexcept;
    bool check(core::token_type type) const noexcept;
    bool match(std::initializer_list<core::token_type> types) noexcept;
    const core::token& consume(core::token_type type, core::error_code code);

    ast::stmt_ptr declaration();
    ast::stmt_ptr statement();
    ast::stmt_ptr var_declaration(core::type type, const core::token& name);
    ast::stmt_ptr func_declaration(core::type return_type, const core::token& name);
    ast::stmt_ptr while_statement();
	ast::stmt_ptr for_statement();
    ast::stmt_ptr if_statement();
    ast::stmt_ptr block_statement();
    ast::stmt_ptr return_statement();

    ast::expression parse_binary(
        std::initializer_list<core::token_type> operators,
        std::function<ast::expression()> sub_parser);
    ast::expression expression();
    ast::expression equality();
    ast::expression assignment();
    ast::expression logic_or();
    ast::expression logic_and();
    ast::expression comparison();
    ast::expression term();
    ast::expression factor();
    ast::expression unary();
    ast::expression postfix();
	ast::expression array_literal();
    ast::expression primary();
    ast::expression finish_call(const core::token& callee);
    ast::expression finish_index(ast::expression object);
    ast::func_param parse_param();

    void synchronize();

    const std::vector<core::token>& tokens_;
    core::error_reporter& reporter_;
    size_t current_ = 0;
};

} // namespace parser