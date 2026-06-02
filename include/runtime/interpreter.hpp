// runtime/interpreter.hpp


#pragma once
#include "runtime/value.hpp"
#include "ast/statement.hpp"
#include "ast/expression.hpp"
#include "core/error/error_report.hpp"
#include "runtime/environment.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

namespace runtime {

struct return_exception {
    value return_value_{};
};

class interpreter {
public:
    interpreter(core::error_reporter& reporter, bool debug = false);

    void interpret(const std::vector<std::unique_ptr<ast::statement>>& statements);

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

    value evaluate(const ast::expression& expr);
    value evaluate_literal(const ast::literal_expr& expr);
    value evaluate_variable(const ast::variable_expr& expr);
    value evaluate_assignment(const ast::binary_expr& expr);
    value evaluate_logical(const ast::binary_expr& expr);
    value evaluate_arithmetic(const ast::binary_expr& expr);
    value evaluate_binary(const ast::binary_expr& expr);
    value evaluate_unary(const ast::unary_expr& expr);
	value evaluate_postfix(const ast::postfix_expr& expr);
    value evaluate_call(const ast::call_expr& expr);
	value evaluate_array_literal(const ast::array_literal_expr& expr);
	value evaluate_index(const ast::index_expr& expr);

	value default_value(const core::type& type);

    template <typename T, typename... Args>
    [[noreturn]] void throw_error(core::error_code code, const T& t, Args&&... args) {
        reporter_.error(t.line_, t.column_, code, std::forward<Args>(args)...);
        throw core::interpret_error{ code };
    }

    template <typename T, typename... Args>
    void report_error(core::error_code code, const T& t, Args&&... args) {
        reporter_.error(t.line_, t.column_, code, std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    void error_if(bool condition, core::error_code code, const T& t, Args&&... args) {
        if (condition) {
            throw_error(code, t, std::forward<Args>(args)...);
        }
    }

    core::error_reporter& reporter_;
    std::unique_ptr<environment> global_env_;
    environment* current_env_;
    std::unordered_map<std::string, const ast::func_declaration*> functions_;
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