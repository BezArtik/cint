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
#include <cassert>

namespace runtime {

using tt = core::token_type;
using err = core::error_code;

interpreter::interpreter(core::error_reporter& reporter, bool debug)
    : reporter_(reporter)
    , global_env_(std::make_unique<environment>())
    , current_env_(global_env_.get())
    , debug_(debug) {

	for (const auto& def : core::builtins) 
		global_env_->define_builtin(std::string{def.name_}, def.impl_);
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
        
	auto init_val = default_value(stmt.type_);

    if (stmt.initializer_) {
        auto init = evaluate(*stmt.initializer_);

        if (stmt.type_.is_array() && init.as_array()) {
            init_val = std::move(init);
        } else if (stmt.type_.is_assignable_from(init.type())) {
            if (stmt.type_ == core::type::double_type() && init.type() == core::type::int_type()) {
                init_val = core::value(static_cast<double>(init.as_int().value()));
            } else {
                init_val = std::move(init);
            }
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
        if (!cond.as_bool().value()) break;
        execute(*stmt.body_);
    }
}

void interpreter::execute_for(const ast::for_stmt& stmt) {
	scope_guard guard(current_env_);
	if (stmt.initializer_) execute(*stmt.initializer_);
	while (true) {
		if (stmt.condition_) {
			auto cond = evaluate(*stmt.condition_);
			if (!cond.as_bool().value()) break;
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
    core::value ret_val;
	stmt.value_ ? ret_val = evaluate(*stmt.value_) : ret_val = core::value();
    throw return_exception{ std::move(ret_val) };
}

void interpreter::execute_func_declaration(const ast::func_declaration& stmt) {
    std::string name{ stmt.name_.lexeme_ };
    functions_[name] = &stmt;
}

core::value interpreter::evaluate(const ast::expression& expr) {
    auto result = std::visit(core::overloaded{
        [this](const ast::literal_expr& e)                        { return evaluate_literal(e); },
        [this](const ast::variable_expr& e)                       { return evaluate_variable(e); },
        [this](const std::unique_ptr<ast::binary_expr>& e)        { return evaluate_binary(*e); },
        [this](const std::unique_ptr<ast::unary_expr>& e)         { return evaluate_unary(*e); },
        [this](const std::unique_ptr<ast::postfix_expr>& e)       { return evaluate_postfix(*e); },
        [this](const std::unique_ptr<ast::call_expr>& e)          { return evaluate_call(*e); },
		[this](const std::unique_ptr<ast::array_literal_expr>& e) { return evaluate_array_literal(*e); },
		[this](const std::unique_ptr<ast::index_expr>& e)         { return evaluate_index(*e); },
        }, expr);
    if (debug_) {
        std::cerr << "  → ";
        debug::print_value(result);
    }
    return result;
}

core::value interpreter::evaluate_literal(const ast::literal_expr& expr) {
    const auto& token = expr.value_;
    if (token.type_ == tt::NUMBER) {
        auto lex = token.lexeme_;
        if (token.is_double_literal()) {
            core::value::double_t d;
            auto [ptr, ec] = std::from_chars(lex.data(), lex.data() + lex.size(), d);
            error_if(ec != std::errc(), err::unexpected_literal, token);
            return core::value(d);
        } else {
            core::value::int_t i;
            auto [ptr, ec] = std::from_chars(lex.data(), lex.data() + lex.size(), i);
            error_if(ec != std::errc(), err::unexpected_literal, token);
            return core::value(i);
        }
    }
    if (token.is_string_literal()) {
        auto lex = token.lexeme_;
        core::value::string_t s{ lex.substr(1, lex.size() - 2) };
        return core::value(std::move(s));
    }
    if (token.is_keyword()) {
        auto kw = token.as_keyword();
        if (kw && kw->lexeme_ == "true")  return core::value(true);
        if (kw && kw->lexeme_ == "false") return core::value(false);
    }
    throw_error(err::unexpected_literal, token);
}

core::value interpreter::evaluate_variable(const ast::variable_expr& expr) {
    std::string name{ expr.name_.lexeme_ };
    auto val = current_env_->get(name);
    error_if(!val, err::undefined_variable, expr, name);
    return *val;
}

core::value interpreter::evaluate_binary(const ast::binary_expr& expr) {
    switch (expr.op_.type_) {
    case tt::EQUAL: case tt::PLUS_EQUAL:
    case tt::MINUS_EQUAL: case tt::STAR_EQUAL:
    case tt::SLASH_EQUAL: case tt::PERCENT_EQUAL:
        return evaluate_assignment(expr);

    case tt::AND: case tt::OR:
        return evaluate_logical(expr);

    default:
        return evaluate_arithmetic(expr);
    }
}

core::value interpreter::evaluate_assignment(const ast::binary_expr& expr) {
    if (auto* idx = std::get_if<std::unique_ptr<ast::index_expr>>(&expr.left_)) {
        const auto& var = std::get<ast::variable_expr>((*idx)->object_);
        std::string name{ var.name_.lexeme_ };
        return evaluate_index_assignment(expr, **idx, name);
    }

    const auto& var = std::get<ast::variable_expr>(expr.left_);
    std::string name{ var.name_.lexeme_ };
    return evaluate_simple_assignment(expr, var, name);
}

core::value interpreter::evaluate_simple_assignment(const ast::binary_expr& expr,
                                                    const ast::variable_expr& var,
                                                    const std::string& name) {
    auto right = evaluate(expr.right_);

    if (expr.op_.type_ == tt::EQUAL) {
        current_env_->assign(name, right);
        return right;
    }

    auto left = evaluate_variable(var);
    core::value result;
    switch (expr.op_.type_) {
    case tt::PLUS_EQUAL:    result = left.add(right); break;
    case tt::MINUS_EQUAL:   result = left.sub(right); break;
    case tt::STAR_EQUAL:    result = left.mul(right); break;
    case tt::SLASH_EQUAL:   result = left.div(right); break;
    case tt::PERCENT_EQUAL: result = left.mod(right); break;
    default: break;
    }
    current_env_->assign(name, result);
    return result;
}

core::value interpreter::evaluate_index_assignment(const ast::binary_expr& expr,
                                                   const ast::index_expr& idx,
                                                   const std::string& name) {
    auto* arr_ptr = current_env_->get_mut(name);
    if (!arr_ptr) throw_error(err::undefined_variable, expr, name);
        
    auto index_val = evaluate(idx.index_);
    auto right = evaluate(expr.right_);

    auto i = index_val.as_int().value();
    auto size = arr_ptr->as_array()->size();
    error_if(i < 0 || i >= size, err::index_out_of_bounds, expr, name, i);

    auto vec = std::move(*arr_ptr->as_array_mut());
    auto& element = vec[i];

    core::value result;
    if (expr.op_.type_ == tt::EQUAL) {
        result = std::move(right);
    } else {
        auto left_val = element;
        switch (expr.op_.type_) {
        case tt::PLUS_EQUAL:    result = left_val.add(right); break;
        case tt::MINUS_EQUAL:   result = left_val.sub(right); break;
        case tt::STAR_EQUAL:    result = left_val.mul(right); break;
        case tt::SLASH_EQUAL:   result = left_val.div(right); break;
        case tt::PERCENT_EQUAL: result = left_val.mod(right); break;
        default: break;
        }
    }

    element = std::move(result);
    *arr_ptr = core::value(std::move(vec));
    return element;
}

core::value interpreter::evaluate_logical(const ast::binary_expr& expr) {
    auto left = evaluate(expr.left_);
    if (expr.op_.type_ == tt::AND) {
        if (!left.as_bool().value()) return core::value(false);
    } else {
        if (left.as_bool().value()) return core::value(true);
    }
    auto right = evaluate(expr.right_);
    return core::value(right.as_bool().value());
}

core::value interpreter::evaluate_arithmetic(const ast::binary_expr& expr) {
    auto left = evaluate(expr.left_);
    auto right = evaluate(expr.right_);
    try {
        switch (expr.op_.type_) {
        case tt::PLUS:          return left.add(right);
        case tt::MINUS:         return left.sub(right);
        case tt::STAR:          return left.mul(right);
        case tt::SLASH:         return left.div(right);
        case tt::PERCENT:       return left.mod(right);
        case tt::EQUAL_EQUAL:   return left.eq(right);
        case tt::BANG_EQUAL:    return left.neq(right);
        case tt::LESS:          return left.lt(right);
        case tt::LESS_EQUAL:    return left.le(right);
        case tt::GREATER:       return left.gt(right);
        case tt::GREATER_EQUAL: return left.ge(right);
        default: throw_error(err::unsupported_binary_operator, expr, expr.op_.lexeme_);
        }
    } catch (const core::interpret_error& e) {
        throw_error(e.code_, expr, expr.op_.lexeme_);
    }
}

core::value interpreter::evaluate_unary(const ast::unary_expr& expr) {
    auto operand = evaluate(expr.operand_);

    switch (expr.op_.type_) {
    case tt::MINUS: {
        if (operand.type() == core::type::int_type())
            return core::value(-operand.as_int().value());
        return core::value(-operand.as_double().value());
    }
    case tt::BANG:
        return operand.not_op();

    case tt::INCREMENT:
    case tt::DECREMENT: {
        const auto& var = std::get<ast::variable_expr>(expr.operand_);
        std::string name{ var.name_.lexeme_ };
        auto old_val = evaluate_variable(var);

        core::value new_val;
        if (old_val.type() == core::type::int_type()) {
            auto v = old_val.as_int().value();
            new_val = core::value(expr.op_.type_ == tt::INCREMENT ? v + 1 : v - 1);
        } else {
            auto v = old_val.as_double().value();
            new_val = core::value(expr.op_.type_ == tt::INCREMENT ? v + 1.0 : v - 1.0);
        }
        error_if(!current_env_->assign(name, new_val), err::undefined_variable, expr, name);
        return new_val;
    }

    default:
        throw_error(err::unsupported_unary_operator, expr, expr.op_.lexeme_);
    }
}

core::value interpreter::evaluate_postfix(const ast::postfix_expr& expr) {
    const auto& var = std::get<ast::variable_expr>(expr.operand_);
    std::string name{ var.name_.lexeme_ };
    auto old_val = evaluate_variable(var);

    core::value new_val;
    if (old_val.type() == core::type::int_type()) {
        auto v = old_val.as_int().value();
        new_val = core::value(expr.op_.type_ == tt::INCREMENT ? v + 1 : v - 1);
    } else {
        auto v = old_val.as_double().value();
        new_val = core::value(expr.op_.type_ == tt::INCREMENT ? v + 1.0 : v - 1.0);
    }
    current_env_->assign(name, new_val);
    return old_val;
}

core::value interpreter::evaluate_call(const ast::call_expr& expr) {
    std::string name{ expr.callee_.lexeme_ };

    auto builtin = current_env_->get_builtin(name);
    auto func_it = functions_.find(name);

    if (!builtin && func_it == functions_.end()) {
        report_error(err::undefined_function, expr, name);
        return core::value();
    }

    std::vector<core::value> args;
    args.reserve(expr.args_.size());

    try {
        std::ranges::transform(expr.args_, std::back_inserter(args),
            [this](const auto& arg) { return evaluate(arg); });
    } catch (const core::interpret_error& e) {
        return core::value();
    }

    if (builtin) {
        try {
            return (*builtin)(args);
        } catch (const core::interpret_error&) {
            return core::value();
        }
    }

    const auto& func = *func_it->second;

    if (args.size() != func.params_.size()) {
        throw_error(err::argument_count_mismatch, expr, name,
            std::to_string(func.params_.size()),
            std::to_string(args.size()));
        return core::value();
    }

    scope_guard guard(current_env_);

    for (size_t i = 0; i < func.params_.size(); ++i) {
        std::string param_name{ func.params_[i].name_.lexeme_ };
        current_env_->define(param_name, std::move(args[i]));
    }

    auto result = default_value(func.return_type_);

    try {
        for (const auto& s : func.body_->statements_) execute(*s);
    } catch (const return_exception& ret) {
        result = ret.return_value_;
    } catch (const core::interpret_error& e) {
        return core::value();
    }
    return result;
}

core::value interpreter::evaluate_array_literal(const ast::array_literal_expr& expr) {
    std::vector<core::value> elements;
    elements.reserve(expr.elements_.size());
    std::ranges::transform(expr.elements_, std::back_inserter(elements),
        [this](const auto& elem) { return evaluate(elem); });
    return core::value(std::move(elements));
}

core::value interpreter::evaluate_index(const ast::index_expr& expr) {
    auto obj = evaluate(expr.object_);
    auto idx = evaluate(expr.index_);

    error_if(!obj.as_array(), err::indexing_non_array, expr);

    auto i = idx.as_int().value();
    auto size = static_cast<int64_t>(obj.as_array()->size());

    error_if(i < 0 || i >= size, err::index_out_of_bounds, expr);

    return (*obj.as_array())[static_cast<size_t>(i)];
}

core::value interpreter::default_value(const core::type& type) {
    if (type == core::type::int_type())         return core::value(int64_t{ 0 });
    if (type == core::type::double_type())      return core::value(0.0);
    if (type == core::type::bool_type())        return core::value(false);
    if (type == core::type::string_type())      return core::value(std::string(""));
    if (type.is_void())                         return core::value();
    if (type.is_array())                        return core::value(std::vector<core::value>{});
	if (type.is_unknown()) throw core::interpret_error{ err::unexpected_literal };
	return core::value();
}

} // namespace runtime