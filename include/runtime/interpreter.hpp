// runtime/interpreter.hpp


#pragma once
#include "core/value/value.hpp"
#include "ast/statement.hpp"
#include "ast/expression.hpp"
#include "core/error/error_report.hpp"
#include "core/utils/hash.hpp"
#include "runtime/environment.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

namespace runtime {

struct return_exception {
    core::value return_value_{};
};

class interpreter {
public:
    interpreter(core::error_reporter& reporter, bool debug = false);

    void interpret(const std::vector<ast::stmt_ptr>& statements);

private:

    void execute(const ast::statement& stmt);
    void execute_expression_stmt(const ast::expression_stmt& stmt);
    void execute_var_declaration(const ast::var_declaration& stmt);
    void execute_block(const ast::block_stmt& stmt);
    void execute_while(const ast::while_stmt& stmt);
	void execute_for(const ast::for_stmt& stmt);
    void execute_if(const ast::if_stmt& stmt);
    void execute_return_stmt(const ast::return_stmt& stmt);
    void execute_func_declaration(const ast::func_declaration& stmt);

    core::value evaluate(const ast::expression& expr);
    core::value evaluate_literal(const ast::literal_expr& expr);
    core::value evaluate_variable(const ast::variable_expr& expr);
    core::value evaluate_assignment(const ast::binary_expr& expr);
    core::value evaluate_simple_assignment(const ast::binary_expr& expr, 
                                     const ast::variable_expr& var);                         
    core::value evaluate_index_assignment(const ast::binary_expr& expr,
                                    const ast::index_expr& idx);                           
    core::value evaluate_logical(const ast::binary_expr& expr);
    core::value evaluate_arithmetic(const ast::binary_expr& expr);
    core::value evaluate_binary(const ast::binary_expr& expr);
    core::value evaluate_unary(const ast::unary_expr& expr);
	core::value evaluate_postfix(const ast::postfix_expr& expr);
    core::value evaluate_call(const ast::call_expr& expr);
	core::value evaluate_array_literal(const ast::array_literal_expr& expr);
	core::value evaluate_index(const ast::index_expr& expr);

	core::value default_value(const core::type& type);

    core::error_reporter& reporter_;
    std::unique_ptr<environment> global_env_;
    environment* current_env_;
    std::unordered_map<
        std::string, const ast::func_declaration*,
        core::string_hash, std::equal_to<>
    > functions_;
    bool debug_ = false;

    class scope_guard {
	public:
        scope_guard(environment* env);
        ~scope_guard();
		scope_guard(const scope_guard&) = delete;
		scope_guard& operator=(const scope_guard&) = delete;
    private:
		environment* env_;
    };
};

} // namespace runtime