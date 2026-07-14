// runtime/interpreter.hpp

#pragma once
#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/error/error_report.hpp"
#include "core/token/token_types.hpp"
#include "core/utils/builtins.hpp"
#include "core/utils/scoped_map.hpp"
#include "core/value/value.hpp"

#include <string_view>
#include <vector>

namespace runtime {

class interpreter {
public:
    interpreter(core::error_reporter& reporter, bool debug = false);

    void interpret(const std::vector<ast::stmt_ptr>& statements);

private:
    void execute(const ast::statement& stmt);
    void execute_expression_stmt(const ast::expression_stmt& stmt);
    void execute_var_declaration(const ast::var_declaration& stmt);
    void execute_block(const ast::block_stmt& stmt, bool create_scope = true);
    void execute_body(const ast::statement& body);
    void execute_while(const ast::while_stmt& stmt);
    void execute_for(const ast::for_stmt& stmt);
    void execute_if(const ast::if_stmt& stmt);
    void execute_return_stmt(const ast::return_stmt& stmt);
    void execute_func_declaration(const ast::func_declaration& stmt);

    core::value evaluate(const ast::expression& expr);
    core::value evaluate_literal(const ast::literal_expr& expr);
    core::value evaluate_variable(const ast::variable_expr& expr);
    core::value evaluate_assignment(const ast::assignment_expr& expr);
    core::value evaluate_binary(const ast::binary_expr& expr);
    core::value evaluate_unary(const ast::unary_expr& expr);
    core::value evaluate_postfix(const ast::postfix_expr& expr);
    core::value evaluate_call(const ast::call_expr& expr);
    core::value evaluate_array_literal(const ast::array_literal_expr& expr);
    core::value evaluate_index(const ast::index_expr& expr);
    core::value& evaluate_lvalue(const ast::expression& expr);

    struct runtime_var {
        const core::type* static_type_;
        core::value value_;
    };

    using callable = std::variant<const ast::func_declaration*, core::builtin_fn_ptr>;
    struct function_entry {
        std::string_view name_;
        callable impl_;
    };

    core::error_reporter& reporter_;
    core::scoped_map<runtime_var> values_;
    std::vector<function_entry> functions_;
    uint32_t recursion_depth_ = 0;
    bool debug_;
    static constexpr uint32_t MAX_RECURSION_DEPTH = 250;
    static constexpr uint32_t MAX_ARGUMENTS = 16;
};

}  // namespace runtime
