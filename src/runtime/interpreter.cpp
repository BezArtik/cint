// runtime/interpreter.cpp


#include "runtime/interpreter.hpp"
#include "runtime/environment.hpp"
#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/utils/overloaded.hpp"
#include "core/utils/builtins.hpp"
#include "core/token/token_types.hpp"
#include "core/token/keywords.hpp"
#include "core/error/error_codes.hpp"
#include "semantics/type_check.hpp"
#include "debug/debug.hpp"
#include <stdexcept>
#include <string>
#include <iostream>
#include <cmath>
#include <iterator>
#include <charconv>
#include <utility>
#include <algorithm>

namespace runtime {

[[noreturn]] void interpreter::error(core::error_code code, size_t line, size_t column, std::string_view msg) {
	reporter_.error(line, column, code, msg);
    throw core::interpret_error{ code };
}

interpreter::interpreter(core::error_reporter& reporter, bool debug)
    : reporter_(reporter)
    , global_env_(std::make_unique<environment>())
    , current_env_(global_env_.get())
    , debug_(debug) {

    for (const auto& def : core::builtins()) 
        global_env_->define_builtin(def.name_, def.impl_);
}

interpreter::scope_guard::scope_guard(environment* env) : env_(env) {
    env_->push_scope();
}
interpreter::scope_guard::~scope_guard() {
    env_->pop_scope();
}

void interpreter::interpret(const std::vector<std::unique_ptr<ast::statement>>& statements) {
    try {
        for (const auto& stmt : statements) {
            if (std::holds_alternative<ast::func_declaration>(stmt->data_)) {
                execute(*stmt);
            }
        }
        for (const auto& stmt : statements) {
            if (!std::holds_alternative<ast::func_declaration>(stmt->data_)) {
                execute(*stmt);
            }
        }
        auto main_it = functions_.find("main");
        if (main_it != functions_.end()) {
            const auto& func = *main_it->second;
            scope_guard guard(current_env_);
            try {
                for (const auto& s : func.body_->statements_) execute(*s);
            } catch (const return_exception& ret) {
                if (debug_) {
                    std::cerr << "[main] returned ";
                    debug::print_value(ret.return_value_);
                }
            }
        }

    } catch (const core::interpret_error& e) {}
}

void interpreter::execute(const ast::statement& stmt) {
    if (debug_) debug::print_execution("Executing statement...");
    std::visit(core::overloaded{
        [this](const ast::expression_stmt& s) { execute_expression_stmt(s); },
        [this](const ast::var_declaration& s) { execute_var_declaration(s); },
        [this](const ast::block_stmt& s) { execute_block(s); },
        [this](const ast::while_stmt& s) { execute_while(s); },
		[this](const ast::for_stmt& s) { execute_for(s); },
        [this](const ast::if_stmt& s) { execute_if(s); },
        [this](const ast::return_stmt& s) { execute_return_stmt(s); },
        [this](const ast::func_declaration& s) { execute_func_declaration(s); },
        }, stmt.data_);
}

void interpreter::execute_expression_stmt(const ast::expression_stmt& stmt) {
    evaluate(stmt.expr_);
}

void interpreter::execute_var_declaration(const ast::var_declaration& stmt) {
    std::string name{ stmt.name_.lexeme_ };
    if (debug_) {
        debug::print_execution("var " + name + " : " + std::string(debug::type_name(stmt.type_)));
    }
        
    value init_val;
    if (stmt.type_ == core::type::int_type())         init_val = value(int64_t{ 0 });
    else if (stmt.type_ == core::type::double_type()) init_val = value(0.0);
    else if (stmt.type_ == core::type::bool_type())   init_val = value(false);
    else if (stmt.type_ == core::type::string_type()) init_val = value(std::string(""));
    else if (stmt.type_.is_void())                    init_val = value();
    else error(core::error_code::unknown_type, stmt.name_.line_, stmt.name_.column_);

    if (stmt.initializer_) {
        auto init = evaluate(*stmt.initializer_);
        if (stmt.type_.is_assignable_from(init.type())) {
            if (stmt.type_ == core::type::double_type() && init.type() == core::type::int_type()) {
                init_val = value(static_cast<double>(init.as_int().value()));
            } else {
                init_val = std::move(init);
            }
        } else {
            error(core::error_code::type_mismatch_initialization, stmt.name_.line_, stmt.name_.column_, name);
        }
    }

    current_env_->define(name, std::move(init_val));
}

void interpreter::execute_block(const ast::block_stmt& stmt) {
    scope_guard guard(current_env_);
    for (const auto& s : stmt.statements_) execute(*s);
}

void interpreter::execute_while(const ast::while_stmt& stmt) {
    while (true) {
        auto cond = evaluate(stmt.condition_);
        if (!cond.as_bool().value_or(false)) break;
        execute(*stmt.body_);
    }
}

void interpreter::execute_for(const ast::for_stmt& stmt) {
	scope_guard guard(current_env_);
	if (stmt.initializer_) {
		execute(*stmt.initializer_);
	}
	while (true) {
		if (stmt.condition_) {
			auto cond = evaluate(*stmt.condition_);
			if (!cond.as_bool().value_or(false)) break;
		}
		execute(*stmt.body_);
		if (stmt.increment_) evaluate(*stmt.increment_);
	}
}

void interpreter::execute_if(const ast::if_stmt& stmt) {
    auto cond = evaluate(stmt.condition_);
    if (cond.as_bool().value_or(false)) {
        execute(*stmt.then_branch_);
    } else if (stmt.else_branch_) {
        execute(*stmt.else_branch_);
    }
}

void interpreter::execute_return_stmt(const ast::return_stmt& stmt) {
    value ret_val;
	stmt.value_ ? ret_val = evaluate(*stmt.value_) : ret_val = value();
    throw return_exception{ std::move(ret_val) };
}

void interpreter::execute_func_declaration(const ast::func_declaration& stmt) {
    std::string name{ stmt.name_.lexeme_ };
    functions_[name] = &stmt;
}

value interpreter::evaluate(const ast::expression& expr) {
    auto result = std::visit(core::overloaded{
        [this](const ast::literal_expr& e) { return evaluate_literal(e); },
        [this](const ast::variable_expr& e) { return evaluate_variable(e); },
        [this](const std::unique_ptr<ast::binary_expr>& e) { return evaluate_binary(*e); },
        [this](const std::unique_ptr<ast::unary_expr>& e) { return evaluate_unary(*e); },
        [this](const std::unique_ptr<ast::postfix_expr>& e) { return evaluate_postfix(*e); },
        [this](const std::unique_ptr<ast::call_expr>& e) { return evaluate_call(*e); },
        }, expr);
    if (debug_) {
        std::cerr << "  → ";
        debug::print_value(result);
    }
    return result;
}

value interpreter::evaluate_literal(const ast::literal_expr& expr) {
    const auto& token = expr.value_;
    if (token.type_ == core::token_type::NUMBER) {
        auto lex = token.lexeme_;
        if (token.is_double_literal()) {
            double d;
            auto [ptr, ec] = std::from_chars(lex.data(), lex.data() + lex.size(), d);
            if (ec != std::errc())
                error(core::error_code::unexpected_literal, token.line_, token.column_);
            return value(d);
        } else {
            int64_t i;
            auto [ptr, ec] = std::from_chars(lex.data(), lex.data() + lex.size(), i);
            if (ec != std::errc())
                error(core::error_code::unexpected_literal, token.line_, token.column_);
            return value(i);
        }
    }
    if (token.type_ == core::token_type::STRING) {
        auto lex = token.lexeme_;
        std::string s{ lex.substr(1, lex.size() - 2) };
        return value(std::move(s));
    }
    if (token.is_keyword()) {
        auto kw = token.as_keyword();
        if (kw && kw->lexeme_ == "true")  return value(true);
        if (kw && kw->lexeme_ == "false") return value(false);
    }
    error(core::error_code::unexpected_literal, token.line_, token.column_);
}

value interpreter::evaluate_variable(const ast::variable_expr& expr) {
    std::string name{ expr.name_.lexeme_ };
    auto val = current_env_->get(name);
    if (!val) error(core::error_code::undefined_variable, expr.name_.line_, expr.name_.column_, name);
    return *val;
}

value interpreter::evaluate_binary(const ast::binary_expr& expr) {
    switch (expr.op_.type_) {
    case core::token_type::EQUAL:
    case core::token_type::PLUS_EQUAL:
    case core::token_type::MINUS_EQUAL:
    case core::token_type::STAR_EQUAL:
    case core::token_type::SLASH_EQUAL:
    case core::token_type::PERCENT_EQUAL:
        return evaluate_assignment(expr);

    case core::token_type::AND:
    case core::token_type::OR:
        return evaluate_logical(expr);

    default:
        return evaluate_arithmetic(expr);
    }
}

value interpreter::evaluate_assignment(const ast::binary_expr& expr) {
    const auto& var = std::get<ast::variable_expr>(expr.left_);
    std::string name{ var.name_.lexeme_ };
    auto right = evaluate(expr.right_);

    if (expr.op_.type_ == core::token_type::EQUAL) {
        if (!current_env_->assign(name, right))
            error(core::error_code::undefined_variable, expr.op_.line_, expr.op_.column_, name);
        return right;
    }

    auto left = evaluate_variable(var);
    value result;
    try {
        switch (expr.op_.type_) {
        case core::token_type::PLUS_EQUAL:    result = left.add(right); break;
        case core::token_type::MINUS_EQUAL:   result = left.sub(right); break;
        case core::token_type::STAR_EQUAL:    result = left.mul(right); break;
        case core::token_type::SLASH_EQUAL:   result = left.div(right); break;
        case core::token_type::PERCENT_EQUAL: result = left.mod(right); break;
        default: error(core::error_code::unsupported_binary_operator, expr.op_.line_, expr.op_.column_);
        }
    } catch (const core::interpret_error& e) {
        error(e.code_, expr.op_.line_, expr.op_.column_);
    }

    if (!current_env_->assign(name, result))
        error(core::error_code::undefined_variable, expr.op_.line_, expr.op_.column_, name);
    return result;
}

value interpreter::evaluate_logical(const ast::binary_expr& expr) {
    auto left = evaluate(expr.left_);

    if (expr.op_.type_ == core::token_type::AND) {
        if (!left.as_bool().value_or(false)) return value(false);
    } else { 
        if (left.as_bool().value_or(false)) return value(true);
    }

    auto right = evaluate(expr.right_);
    return value(right.as_bool().value_or(false));
}

value interpreter::evaluate_arithmetic(const ast::binary_expr& expr) {
    auto left = evaluate(expr.left_);
    auto right = evaluate(expr.right_);
    try {
        switch (expr.op_.type_) {
        case core::token_type::PLUS:          return left.add(right);
        case core::token_type::MINUS:         return left.sub(right);
        case core::token_type::STAR:          return left.mul(right);
        case core::token_type::SLASH:         return left.div(right);
        case core::token_type::PERCENT:       return left.mod(right);
        case core::token_type::EQUAL_EQUAL:   return left.eq(right);
        case core::token_type::BANG_EQUAL:    return left.neq(right);
        case core::token_type::LESS:          return left.lt(right);
        case core::token_type::LESS_EQUAL:    return left.le(right);
        case core::token_type::GREATER:       return left.gt(right);
        case core::token_type::GREATER_EQUAL: return left.ge(right);
        default:
            error(core::error_code::unsupported_binary_operator,
                expr.op_.line_, expr.op_.column_);
        }
    } catch (const core::interpret_error& e) {
        error(e.code_, expr.op_.line_, expr.op_.column_);
    }
}

value interpreter::evaluate_unary(const ast::unary_expr& expr) {
    auto operand = evaluate(expr.operand_);

    try {
        switch (expr.op_.type_) {
        case core::token_type::MINUS: {
            if (operand.type() == core::type::int_type())
                return value(-operand.as_int().value());
            return value(-operand.as_double().value());
        }
        case core::token_type::BANG:
            return operand.not_op();

        case core::token_type::INCREMENT:
        case core::token_type::DECREMENT: {
            const auto* var = std::get_if<ast::variable_expr>(&expr.operand_);
            if (!var) {
                error(core::error_code::increment_requires_lvalue,
                    expr.op_.line_, expr.op_.column_);
            }

            std::string name{ var->name_.lexeme_ };
            auto old_val = evaluate_variable(*var);

            value new_val;
            if (old_val.type() == core::type::int_type()) {
                auto v = old_val.as_int().value();
                new_val = value(expr.op_.type_ == core::token_type::INCREMENT
                    ? v + 1 : v - 1);
            } else {
                auto v = old_val.as_double().value();
                new_val = value(expr.op_.type_ == core::token_type::INCREMENT
                    ? v + 1.0 : v - 1.0);
            }
            if (!current_env_->assign(name, new_val))
                error(core::error_code::undefined_variable,
                    expr.op_.line_, expr.op_.column_, name);
            return new_val;
        }

        default:
            error(core::error_code::unsupported_unary_operator,
                expr.op_.line_, expr.op_.column_);
        }
    } catch (const core::interpret_error& e) {
        error(e.code_, expr.op_.line_, expr.op_.column_);
    }
}

value interpreter::evaluate_postfix(const ast::postfix_expr& expr) {
    const auto* var = std::get_if<ast::variable_expr>(&expr.operand_);
    std::string name{ var->name_.lexeme_ };
    auto old_val = evaluate_variable(*var);

    try {
        value new_val;
        if (old_val.type() == core::type::int_type()) {
            auto v = old_val.as_int().value();
            new_val = value(expr.op_.type_ == core::token_type::INCREMENT ? v + 1 : v - 1);
        } else {
            auto v = old_val.as_double().value();
            new_val = value(expr.op_.type_ == core::token_type::INCREMENT ? v + 1.0 : v - 1.0);
        }
        if (!current_env_->assign(name, new_val))
            error(core::error_code::undefined_variable, expr.op_.line_, expr.op_.column_, name);
    } catch (const core::interpret_error& e) {
        error(e.code_, expr.op_.line_, expr.op_.column_);
    }

    return old_val;
}

value interpreter::evaluate_call(const ast::call_expr& expr) {
    std::string name{ expr.callee_.lexeme_ };

    std::vector<value> args;
    args.reserve(expr.args_.size());
    std::ranges::transform(expr.args_, std::back_inserter(args),
        [this](const auto& arg) { return evaluate(arg); });

    auto builtin = current_env_->get_builtin(name);
    if (builtin) return (*builtin)(args);
        
    auto func_it = functions_.find(name);
    if (func_it == functions_.end()) 
		error(core::error_code::undefined_function, expr.callee_.line_, expr.callee_.column_, name);
    

    const auto& func = *func_it->second;

    scope_guard guard(current_env_);

    for (size_t i = 0; i < func.params_.size(); ++i) {
        std::string param_name{ func.params_[i].name_.lexeme_ };
        auto param_val = (i < args.size()) ? std::move(args[i]) : value();
        current_env_->define(param_name, std::move(param_val));
    }
    
    value result;
    try {
        for (const auto& s : func.body_->statements_) execute(*s);
        if (func.return_type_ == core::type::int_type())         result = value(int64_t{ 0 });
        else if (func.return_type_ == core::type::double_type()) result = value(0.0);
        else if (func.return_type_ == core::type::bool_type())   result = value(false);
        else if (func.return_type_ == core::type::string_type()) result = value(std::string(""));
        else                                                     result = value();
    } catch (const return_exception& ret) {
        result = ret.return_value_;
    }

    return result;
}

} // namespace runtime