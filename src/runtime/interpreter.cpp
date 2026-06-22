// runtime/interpreter.cpp


#include "runtime/interpreter.hpp"
#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/utils/overloaded.hpp"
#include "core/utils/builtins.hpp"
#include "core/utils/scoped_map.hpp"
#include "core/token/token_types.hpp"
#include "core/token/keywords.hpp"
#include "core/error/error_codes.hpp"
#include "debug/debug.hpp"
#include <string>
#include <iostream>
#include <charconv>
#include <utility>
#include <algorithm>
#include <cassert>

namespace runtime {

using tt = core::token_type;
using t = core::type;
using err = core::error_code;

interpreter::interpreter(core::error_reporter& reporter, bool debug)
    : reporter_(reporter), debug_(debug) {
    for (const auto& def : core::builtins)
        functions_.emplace(def.name_, def.impl_);
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
            const auto& func = *std::get<const ast::func_declaration*>(main_it->second);
            core::scope_guard guard(values_);
            try {
                for (const auto& s : func.body_->statements_) execute(*s);
            } catch (const return_exception& ret) {
                if (debug_) {
                    std::cerr << "[main] returned ";
                    debug::print_value(ret.return_value_);
                }
            }
        }

    } catch (const core::interpret_error&) {}
}

bool interpreter::block_has_declarations(const ast::block_stmt& block) const noexcept {
    for (const auto& stmt : block.statements_) {
        if (std::holds_alternative<ast::var_declaration>(stmt->data_)) return true;  
        if (auto* inner = std::get_if<ast::block_stmt>(&stmt->data_)) {
            if (block_has_declarations(*inner)) return true;
        }
    }
    return false;
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
    auto name = stmt.name_.lexeme_;

    auto init_val = default_value(stmt.type_);

    if (stmt.initializer_) {
        auto init = evaluate(*stmt.initializer_);
        init_val = convert(std::move(init), stmt.type_);
    }

    values_.define(name, {stmt.type_, std::move(init_val)});
}

void interpreter::execute_block(const ast::block_stmt& stmt, bool create_scope) {
    std::optional<core::scope_guard<runtime_var>> guard;
    if (create_scope) guard.emplace(values_);
    for (const auto& s : stmt.statements_) execute(*s);
}

void interpreter::execute_loop_body(const ast::statement& body) {
    if (auto* block = std::get_if<ast::block_stmt>(&body.data_)) {
        execute_block(*block, false);
    } else {
        execute(body);
    }
}

void interpreter::execute_while(const ast::while_stmt& stmt) {
    core::scope_guard guard(values_);
    while (true) {
        auto cond = evaluate(stmt.condition_);
        if (!cond.to_bool()) break;
        execute_loop_body(*stmt.body_);
    }
}

void interpreter::execute_for(const ast::for_stmt& stmt) {
    core::scope_guard guard(values_);
    if (stmt.initializer_) execute(*stmt.initializer_);
    while (true) {
        if (stmt.condition_) {
            auto cond = evaluate(*stmt.condition_);
            if (!cond.to_bool()) break;
        }
        execute_loop_body(*stmt.body_);
        if (stmt.increment_) evaluate(*stmt.increment_);
    }
}

void interpreter::execute_if(const ast::if_stmt& stmt) {
    auto cond = evaluate(stmt.condition_);

    auto execute_branch = [this](const ast::statement& branch) {
        if (auto* block = std::get_if<ast::block_stmt>(&branch.data_)) {
            execute_block(*block, block_has_declarations(*block));
        } else {
            execute(branch);
        }
    };

    if (cond.as_bool().value_or(false)) {
        execute_branch(*stmt.then_branch_);
    } else if (stmt.else_branch_) {
        execute_branch(*stmt.else_branch_);
    }
}

void interpreter::execute_return_stmt(const ast::return_stmt& stmt) {
    core::value ret_val;
    stmt.value_ ? ret_val = evaluate(*stmt.value_) : ret_val = core::value();
    throw return_exception{ std::move(ret_val) };
}

void interpreter::execute_func_declaration(const ast::func_declaration& stmt) {
    functions_.emplace(stmt.name_.lexeme_, &stmt);
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
    
    auto to_number = [&](auto&& lex, auto&& num) {
        auto [_, ec] = std::from_chars(lex.data(), lex.data() + lex.size(), num);
        if (ec != std::errc{}) reporter_.interpret_error(token, err::unexpected_literal);
        return num;
    };

    if (token.type_ == tt::NUMBER) {
        auto lex = token.lexeme_;
        if (token.is_double_literal()) {
            core::value::double_t d;
            return core::value(to_number(lex, d));
        } else {
            core::value::int_t i;
            return core::value(to_number(lex, i));
        }
    }
    if (token.is_string_literal()) {
        auto lex = token.lexeme_;
        core::value::string_t s{ lex.substr(1, lex.size() - 2) };
        return core::value(std::move(s));
    }
    auto kw = token.as_keyword();
    return core::value(kw->lexeme_ == "true");
}

core::value interpreter::evaluate_variable(const ast::variable_expr& expr) {
    auto* var = values_.get(expr.name_.lexeme_);
    return var->value_;
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
        return evaluate_index_assignment(expr, **idx);
    }

    const auto& var = std::get<ast::variable_expr>(expr.left_);
    return evaluate_simple_assignment(expr, var);
}

core::value interpreter::evaluate_simple_assignment(
    const ast::binary_expr& expr,
    const ast::variable_expr& var_expr) {
    auto* var = values_.get(var_expr.name_.lexeme_);    
    auto right = evaluate(expr.right_);
    
    if (expr.op_.type_ == tt::EQUAL) {
        var->value_ = convert(std::move(right), var->static_type_);
        return var->value_;
    }
    
    core::value result;
    switch (expr.op_.type_) {
    case tt::PLUS_EQUAL:    result = var->value_.add(right); break;
    case tt::MINUS_EQUAL:   result = var->value_.sub(right); break;
    case tt::STAR_EQUAL:    result = var->value_.mul(right); break;
    case tt::SLASH_EQUAL:   result = var->value_.div(right); break;
    case tt::PERCENT_EQUAL: result = var->value_.mod(right); break;
    default: break;
    }
    
    var->value_ = convert(std::move(result), var->static_type_);
    return var->value_;
}

core::value interpreter::evaluate_index_assignment(
    const ast::binary_expr& expr,
    const ast::index_expr& idx) {
    const auto& var_expr = std::get<ast::variable_expr>(idx.object_);
    auto* var = values_.get(var_expr.name_.lexeme_);    
    auto* arr = var->value_.as_array();    
    
    auto index_val = evaluate(idx.index_);
    auto right = evaluate(expr.right_);
    
    auto i = *index_val.as_int();
    
    if (i < 0 || i >= arr->size()) {
        reporter_.interpret_error(expr, err::index_out_of_bounds);
    }
    
    auto& element = (*arr)[i];
    
    if (expr.op_.type_ == tt::EQUAL) {
        element = convert(std::move(right), var->static_type_.element_type());
        return element;
    } 

    core::value result;
    switch (expr.op_.type_) {
    case tt::PLUS_EQUAL:    result = element.add(right); break;
    case tt::MINUS_EQUAL:   result = element.sub(right); break;
    case tt::STAR_EQUAL:    result = element.mul(right); break;
    case tt::SLASH_EQUAL:   result = element.div(right); break;
    case tt::PERCENT_EQUAL: result = element.mod(right); break;
    default: break;
    }
    
    element = convert(std::move(result), var->static_type_.element_type());
    return element;
}


core::value interpreter::evaluate_logical(const ast::binary_expr& expr) {
    auto left = evaluate(expr.left_);
    if (expr.op_.type_ == tt::AND) {
        if (!left.to_bool()) return core::value(false);
    } else {
        if (left.to_bool()) return core::value(true);
    }
    auto right = evaluate(expr.right_);
    return core::value(right.to_bool());
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
        default: reporter_.interpret_error(expr, err::unsupported_binary_operator, expr.op_.lexeme_);  
        }
    } catch (const core::interpret_error& e) {
        reporter_.interpret_error(expr, e.code_, expr.op_.lexeme_);
    }
}

core::value interpreter::evaluate_unary(const ast::unary_expr& expr) {
    auto operand = evaluate(expr.operand_);

    switch (expr.op_.type_) {
    case tt::MINUS: {
        if (operand.type() == t::int_type())
            return core::value(-operand.to_int());
        return core::value(-operand.to_double());
    }
    case tt::BANG:
        return operand.not_op();

    case tt::INCREMENT:
    case tt::DECREMENT: {
        const auto& var_expr = std::get<ast::variable_expr>(expr.operand_);
        auto* var = values_.get(var_expr.name_.lexeme_);
    
        core::value new_val;
        if (var->static_type_.is_int()) {
            auto v = var->value_.to_int();
            new_val = core::value(expr.op_.type_ == tt::INCREMENT ? v + 1 : v - 1);
        } else {
            auto v = var->value_.to_double();
            new_val = core::value(expr.op_.type_ == tt::INCREMENT ? v + 1.0 : v - 1.0);
        }
    
        var->value_ = new_val;
        return new_val;
    }
    
    default:
        reporter_.interpret_error(expr, err::unsupported_unary_operator, expr.op_.lexeme_);
    
    }
}

core::value interpreter::evaluate_postfix(const ast::postfix_expr& expr) {
    const auto& var_expr = std::get<ast::variable_expr>(expr.operand_);
    auto* var = values_.get(var_expr.name_.lexeme_);
    auto old_val = var->value_;
    
    core::value new_val;
    if (var->static_type_.is_int()) {
        auto v = old_val.to_int();
        new_val = core::value(expr.op_.type_ == tt::INCREMENT ? v + 1 : v - 1);
    } else {
        auto v = old_val.to_double();
        new_val = core::value(expr.op_.type_ == tt::INCREMENT ? v + 1.0 : v - 1.0);
    }
    
    var->value_ = new_val;
    return old_val;
}

core::value interpreter::evaluate_call(const ast::call_expr& expr) {
    auto name = expr.callee_.lexeme_;

    if (recursion_depth_ >= MAX_RECURSION_DEPTH) {
        reporter_.interpret_error(expr, err::stack_overflow);
    }
    recursion_depth_++;

    struct depth_guard {
        uint32_t& d;
        ~depth_guard() { d--; }
    } d_guard{ recursion_depth_ };

    std::vector<core::value> args;
    args.reserve(expr.args_.size());

    try {
        std::ranges::transform(expr.args_, std::back_inserter(args),
            [this](const auto& arg) { return evaluate(arg); });
    } catch (const core::interpret_error&) {
        return core::value();
    }

    auto it = functions_.find(name);
    return std::visit(core::overloaded{
        [&](core::builtin_fn_ptr builtin) -> core::value {
            try {
                return (*builtin)(args);
            } catch (const core::interpret_error& e) {
                reporter_.interpret_error(expr, e.code_, name);
                return core::value();
            }
        },
        [&](const ast::func_declaration* func) -> core::value {
            return call_user_function(*func, args, expr);
        }
    }, it->second);
}


core::value interpreter::call_user_function(
    const ast::func_declaration& func,
    const std::vector<core::value>& args,
    const ast::call_expr& expr) {
    
    core::scope_guard guard(values_);

    for (size_t i = 0; i < func.params_.size(); ++i) {
        auto converted = convert(std::move(args[i]), func.params_[i].type_);
        values_.define(func.params_[i].name_.lexeme_, 
                {func.params_[i].type_, std::move(converted)});
    }

    try {
        for (const auto& s : func.body_->statements_) execute(*s);
    } catch (const return_exception& ret) {
        return ret.return_value_;
    } catch (const core::interpret_error&) {
        return core::value();
    }

    return default_value(func.return_type_);
}

core::value interpreter::evaluate_array_literal(const ast::array_literal_expr& expr) {
    std::vector<core::value> elements;
    elements.reserve(expr.elements_.size());
    std::ranges::transform(expr.elements_, std::back_inserter(elements),
        [this](const auto& elem) { return evaluate(elem); });

    if (elements.empty()) return core::value(core::type::unknown_type(), {});
    auto elem_type = elements[0].type();
    for (auto& e : elements) e = convert(std::move(e), elem_type);

    return core::value(elem_type, std::move(elements));
}

core::value interpreter::evaluate_index(const ast::index_expr& expr) {
    auto obj = evaluate(expr.object_);
    auto idx = evaluate(expr.index_);
    const auto* arr = obj.as_array(); 
    auto i = *idx.as_int();
    
    if (i < 0 || i >= arr->size()) {
        reporter_.interpret_error(expr, err::index_out_of_bounds);
    }
    
    return (*arr)[i]; 
}

core::value interpreter::default_value(const t& type) {
    if (type.is_int())       return core::value(core::value::int_t{ 0 });
    if (type.is_double())    return core::value(core::value::double_t{ 0.0 });
    if (type.is_bool())      return core::value(false);
    if (type.is_string())    return core::value(core::value::string_t(""));
    if (type.is_void())      return core::value();
    if (type.is_array())     return core::value(type.element_type(), core::value::array_t{});
    if (type.is_unknown())   throw core::interpret_error{ err::unexpected_literal };
    return core::value();
}

core::value interpreter::convert(core::value val, const core::type& target) {
    if(val.type() == target) return val;
    if(target.is_int() && val.type().is_double())
        return core::value(static_cast<core::value::int_t>(val.to_double()));
    if(target.is_double() && val.type().is_int())
        return core::value(static_cast<core::value::double_t>(val.to_int()));
    return val;
}

} // namespace runtime
