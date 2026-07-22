// semantics/type_check.hpp

#pragma once
#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/error/error_report.hpp"
#include "core/utils/scoped_map.hpp"
#include "core/utils/symbol_registry.hpp"

#include <optional>
#include <span>

namespace semantics {

class type_checker {
public:
    type_checker(core::error_reporter& reporter, const core::symbol_registry& registry);

    bool check(std::span<const ast::stmt_ptr> statements);

private:
    void check_statement(const ast::statement& stmt);
    void check_expression_stmt(const ast::expression_stmt& stmt);
    void check_var_declaration(const ast::var_declaration& stmt);
    void check_block(const ast::block_stmt& stmt, bool create_scope = false);
    void check_body(const ast::statement& body);
    void check_while(const ast::while_stmt& stmt);
    void check_for(const ast::for_stmt& stmt);
    void check_if(const ast::if_stmt& stmt);
    void check_return_stmt(const ast::return_stmt& stmt);
    void check_struct_declaration(const ast::struct_declaration& stmt);
    void check_func_declaration(const ast::func_declaration& stmt);

    core::type type_of(const ast::expression& expr);
    core::type type_of_literal(const ast::literal_expr& expr);
    core::type type_of_variable(const ast::variable_expr& expr);
    core::type type_of_binary(const ast::binary_expr& expr);
    core::type type_of_assignment(const ast::assignment_expr& expr);
    core::type type_of_unary(const ast::unary_expr& expr);
    core::type type_of_postfix(const ast::postfix_expr& expr);
    core::type type_of_call(const ast::call_expr& expr);
    core::type type_of_array_literal(const ast::array_literal_expr& expr);
    core::type type_of_index(const ast::index_expr& expr);
    core::type type_of_member_access(const ast::member_access_expr& expr);

    bool is_lvalue(const ast::expression& expr);

    core::error_reporter& reporter_;
    const core::symbol_registry& registry_;
    core::scoped_map<core::type> symbols_;
    std::optional<core::type> curr_return_type_;
};

}  // namespace semantics
