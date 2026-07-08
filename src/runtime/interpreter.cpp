// runtime/interpreter.cpp

#include "runtime/interpreter.hpp"

#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/error/error_codes.hpp"
#include "core/token/keywords.hpp"
#include "core/token/token_types.hpp"
#include "core/utils/arena.hpp"
#include "core/utils/builtins.hpp"
#include "core/utils/overloaded.hpp"
#include "core/utils/scoped_map.hpp"
#include "core/value/operations.hpp"
#include "core/value/value.hpp"
#include "debug/debug.hpp"

#include <algorithm>
#include <cassert>
#include <charconv>
#include <iostream>
#include <string>
#include <utility>

namespace runtime {

using tt = core::token_type;
using t = core::type;
using k = core::type::kind;
using err = core::error_code;
namespace op = core::ops;

namespace {

core::value apply_increment(core::value& val, core::token_type op, bool return_old) {
    auto old_val = val;

    if (auto* i = val.as_mut<core::value::int_t>()) {
        val = core::value(op == tt::INCREMENT ? *i + 1 : *i - 1);
    } else if (auto* d = val.as_mut<core::value::double_t>()) {
        val = core::value(op == tt::INCREMENT ? *d + 1.0 : *d - 1.0);
    }

    return return_old ? old_val : val;
}

}  // namespace

interpreter::interpreter(core::error_reporter& reporter, bool debug) : reporter_(reporter), debug_(debug) {
    for (const auto& def : core::builtins) functions_.emplace(def.name_, def.impl_);
}

void interpreter::interpret(const std::vector<ast::stmt_ptr>& statements) {
    try {
        for (const auto& stmt : statements) {
            if (std::holds_alternative<ast::func_declaration>(stmt->data_)) execute(*stmt);
        }
        for (const auto& stmt : statements) {
            if (!std::holds_alternative<ast::func_declaration>(stmt->data_)) execute(*stmt);
        }
        auto main_it = functions_.find("main");
        if (main_it != functions_.end()) {
            const auto& func = *std::get<const ast::func_declaration*>(main_it->second);
            core::scope_guard guard(values_);
            try {
                for (const auto& s : func.body_->statements_) execute(*s);
            } catch (const interpreter::return_exception& ret) {
                if (debug_) {
                    std::cerr << "[main] returned ";
                    debug::print_value(ret.return_value_);
                }
            }
        }

    } catch (const core::interpret_error&) {}
}

void interpreter::execute(const ast::statement& stmt) {
    if (debug_) debug::print_execution("Executing statement...");
    core::visit(core::overloaded{
                    [this](const ast::expression_stmt& s) { execute_expression_stmt(s); },
                    [this](const ast::var_declaration& s) { execute_var_declaration(s); },
                    [this](const ast::block_stmt& s) { execute_block(s); },
                    [this](const ast::while_stmt& s) { execute_while(s); },
                    [this](const ast::for_stmt& s) { execute_for(s); },
                    [this](const ast::if_stmt& s) { execute_if(s); },
                    [this](const ast::return_stmt& s) { execute_return_stmt(s); },
                    [this](const ast::func_declaration& s) { execute_func_declaration(s); },
                },
                stmt.data_);
}

void interpreter::execute_expression_stmt(const ast::expression_stmt& stmt) {
    evaluate(stmt.expr_);
}

void interpreter::execute_var_declaration(const ast::var_declaration& stmt) {
    auto init_val = default_value(stmt.type_);

    if (stmt.initializer_) {
        auto init = evaluate(*stmt.initializer_);
        init_val = convert(std::move(init), stmt.type_);
    }

    values_.define(stmt.name_.lexeme_, {stmt.type_, std::move(init_val)});
}

void interpreter::execute_block(const ast::block_stmt& stmt, bool create_scope) {
    std::optional<core::scope_guard<runtime_var>> guard;
    if (create_scope) guard.emplace(values_);
    for (const auto& s : stmt.statements_) execute(*s);
}

void interpreter::execute_body(const ast::statement& body) {
    bool create_scope = false;
    if (auto* block = std::get_if<ast::block_stmt>(&body.data_)) {
        create_scope = ast::has_declarations(*block);
        execute_block(*block, create_scope);
    } else {
        execute(body);
    }
}

void interpreter::execute_while(const ast::while_stmt& stmt) {
    core::scope_guard guard(values_);
    while (true) {
        auto cond = evaluate(stmt.condition_);
        if (!cond.to_bool()) break;
        execute_body(*stmt.body_);
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
        execute_body(*stmt.body_);
        if (stmt.increment_) evaluate(*stmt.increment_);
    }
}

void interpreter::execute_if(const ast::if_stmt& stmt) {
    auto cond = evaluate(stmt.condition_);

    if (cond.to_bool()) {
        execute_body(*stmt.then_branch_);
    } else if (stmt.else_branch_) {
        execute_body(*stmt.else_branch_);
    }
}

void interpreter::execute_return_stmt(const ast::return_stmt& stmt) {
    core::value ret_val;
    stmt.value_ ? ret_val = evaluate(*stmt.value_) : ret_val = core::value();
    throw interpreter::return_exception{std::move(ret_val)};
}

void interpreter::execute_func_declaration(const ast::func_declaration& stmt) {
    functions_.insert_or_assign(stmt.name_.lexeme_, &stmt);
}

core::value interpreter::evaluate(const ast::expression& expr) {
    auto result = core::visit(
        core::overloaded{
            [this](const ast::literal_expr& e) { return evaluate_literal(e); },
            [this](const ast::variable_expr& e) { return evaluate_variable(e); },
            [this](const core::arena_ptr<ast::binary_expr>& e) { return evaluate_binary(*e); },
            [this](const core::arena_ptr<ast::assignment_expr>& e) { return evaluate_assignment(*e); },
            [this](const core::arena_ptr<ast::unary_expr>& e) { return evaluate_unary(*e); },
            [this](const core::arena_ptr<ast::postfix_expr>& e) { return evaluate_postfix(*e); },
            [this](const core::arena_ptr<ast::call_expr>& e) { return evaluate_call(*e); },
            [this](const core::arena_ptr<ast::array_literal_expr>& e) { return evaluate_array_literal(*e); },
            [this](const core::arena_ptr<ast::index_expr>& e) { return evaluate_index(*e); },
        },
        expr);
    if (debug_) {
        std::cerr << "  → ";
        debug::print_value(result);
    }
    return result;
}

core::value interpreter::evaluate_literal(const ast::literal_expr& expr) {
    const auto& token = expr.value_;

    auto to_number = [&](auto&& lex, auto&& num) {
        auto [ptr, ec] = std::from_chars(lex.data(), lex.data() + lex.size(), num);
        if (ec != std::errc{} || ptr != lex.data() + lex.size())
            reporter_.interpret_error(token, err::unexpected_literal);
        return num;
    };

    if (token.is_number_literal()) {
        auto lex = token.lexeme_;
        if (token.is_double_literal()) {
            core::value::double_t d;
            return core::value(to_number(lex, d));
        }
        core::value::int_t i;
        return core::value(to_number(lex, i));
    }

    if (token.is_string_literal()) {
        auto lex = token.lexeme_;
        return core::value(std::string(lex.substr(1, lex.size() - 2)));
    }

    return core::value(token.as_keyword()->lexeme_ == "true");
}

core::value interpreter::evaluate_variable(const ast::variable_expr& expr) {
    auto* var = values_.get(expr.name_.lexeme_);
    return var->value_;
}

core::value interpreter::evaluate_binary(const ast::binary_expr& expr) {
    switch (expr.op_.type_) {
        case tt::LOGICAL_AND:
        case tt::LOGICAL_OR:
            return evaluate_logical(expr);
        default:
            return evaluate_arithmetic(expr);
    }
}

core::value interpreter::evaluate_assignment(const ast::assignment_expr& expr) {
    auto op = expr.op_.type_;
    auto right = evaluate(expr.value_);

    core::value* target = nullptr;

    if (auto* idx = std::get_if<core::arena_ptr<ast::index_expr>>(&expr.target_)) {
        const auto& var_expr = std::get<ast::variable_expr>((*idx)->object_);
        auto* var = values_.get(var_expr.name_.lexeme_);

        var->value_.as_mut<core::value::array_t>()->detach();
        auto* arr = var->value_.as_mut<core::value::array_t>();

        auto index_val = evaluate((*idx)->index_);
        auto i = *index_val.as<core::value::int_t>();

        if (i < 0 || i >= static_cast<core::value::int_t>((*arr)->size()))
            reporter_.interpret_error(expr, err::index_out_of_bounds);

        target = &(*arr)->at(static_cast<size_t>(i));
    } else {
        const auto& var_expr = std::get<ast::variable_expr>(expr.target_);
        auto* var = values_.get(var_expr.name_.lexeme_);

        if (auto* s = var->value_.as_mut<core::value::string_t>()) s->detach();
        if (auto* a = var->value_.as_mut<core::value::array_t>()) a->detach();

        target = &var->value_;
    }

    if (op == tt::EQUAL) {
        *target = convert(std::move(right), target->type());
        return *target;
    }

    switch (op) {
        case tt::PLUS_EQUAL:
            *target = op::add(*target, right);
            break;
        case tt::MINUS_EQUAL:
            *target = op::sub(*target, right);
            break;
        case tt::STAR_EQUAL:
            *target = op::mul(*target, right);
            break;
        case tt::SLASH_EQUAL:
            *target = op::div(*target, right);
            break;
        case tt::PERCENT_EQUAL:
            *target = op::mod(*target, right);
            break;
        case tt::BIT_AND_EQUAL:
            *target = op::bit_and(*target, right);
            break;
        case tt::BIT_OR_EQUAL:
            *target = op::bit_or(*target, right);
            break;
        case tt::XOR_EQUAL:
            *target = op::bit_xor(*target, right);
            break;
        case tt::SHL_EQUAL:
            *target = op::shl(*target, right);
            break;
        case tt::SHR_EQUAL:
            *target = op::shr(*target, right);
            break;
        default:
            break;
    }

    return *target;
}

core::value interpreter::evaluate_logical(const ast::binary_expr& expr) {
    auto left = evaluate(expr.left_);
    if (expr.op_.type_ == tt::LOGICAL_AND) {
        if (!left.to_bool()) return core::value(false);
    } else {
        if (left.to_bool()) return core::value(true);
    }
    return core::value(evaluate(expr.right_).to_bool());
}

core::value interpreter::evaluate_arithmetic(const ast::binary_expr& expr) {
    auto left = evaluate(expr.left_);
    auto right = evaluate(expr.right_);
    try {
        switch (expr.op_.type_) {
            case tt::PLUS:
                return op::add(left, right);
            case tt::MINUS:
                return op::sub(left, right);
            case tt::STAR:
                return op::mul(left, right);
            case tt::SLASH:
                return op::div(left, right);
            case tt::PERCENT:
                return op::mod(left, right);
            case tt::EQUAL_EQUAL:
                return op::eq(left, right);
            case tt::BANG_EQUAL:
                return op::neq(left, right);
            case tt::LESS:
                return op::lt(left, right);
            case tt::LESS_EQUAL:
                return op::le(left, right);
            case tt::GREATER:
                return op::gt(left, right);
            case tt::GREATER_EQUAL:
                return op::ge(left, right);
            case tt::BIT_AND:
                return op::bit_and(left, right);
            case tt::BIT_OR:
                return op::bit_or(left, right);
            case tt::XOR:
                return op::bit_xor(left, right);
            case tt::SHL:
                return op::shl(left, right);
            case tt::SHR:
                return op::shr(left, right);
            default:
                reporter_.interpret_error(expr, err::unsupported_binary_operator, expr.op_.lexeme_);
        }
    } catch (const core::interpret_error& e) { reporter_.interpret_error(expr, e.code_, expr.op_.lexeme_); }
}

core::value interpreter::evaluate_unary(const ast::unary_expr& expr) {
    auto operand = evaluate(expr.operand_);
    auto op = expr.op_.type_;

    switch (op) {
        case tt::MINUS: {
            if (auto* i = operand.as<core::value::int_t>()) return core::value(-*i);
            return core::value(-operand.to_double());
        }
        case tt::BANG:
            return op::not_op(operand);
        case tt::BIT_NOT:
            return op::bit_not(operand);
        case tt::INCREMENT:
        case tt::DECREMENT: {
            const auto& var_expr = std::get<ast::variable_expr>(expr.operand_);
            auto* var = values_.get(var_expr.name_.lexeme_);
            return apply_increment(var->value_, op, false);
        }
        default:
            reporter_.interpret_error(expr, err::unsupported_unary_operator, expr.op_.lexeme_);
    }
}

core::value interpreter::evaluate_postfix(const ast::postfix_expr& expr) {
    const auto& var_expr = std::get<ast::variable_expr>(expr.operand_);
    auto* var = values_.get(var_expr.name_.lexeme_);
    return apply_increment(var->value_, expr.op_.type_, true);
}

core::value interpreter::evaluate_call(const ast::call_expr& expr) {
    auto name = expr.callee_.lexeme_;

    if (recursion_depth_ >= MAX_RECURSION_DEPTH) reporter_.interpret_error(expr, err::stack_overflow);
    recursion_depth_++;

    struct depth_guard {
        uint32_t& d;
        ~depth_guard() { d--; }
    } d_guard{recursion_depth_};

    std::vector<core::value> args;
    args.reserve(expr.args_.size());

    try {
        std::ranges::transform(expr.args_, std::back_inserter(args), [this](const auto& arg) { return evaluate(arg); });
    } catch (const core::interpret_error&) { return core::value(); }

    auto it = functions_.find(name);
    if (it == functions_.end()) reporter_.interpret_error(expr, err::undefined_function, name);

    const auto& callable = it->second;

    if (auto* builtin = std::get_if<core::builtin_fn_ptr>(&callable)) {
        try {
            return (*builtin)(args);
        } catch (const core::interpret_error& e) {
            reporter_.interpret_error(expr, e.code_, name);
            return core::value();
        }
    }

    auto* func = std::get<const ast::func_declaration*>(callable);
    core::scope_guard guard(values_);

    auto fn_param = func->params_;
    for (size_t i = 0; i < fn_param.size(); ++i) {
        auto& param = fn_param[i];
        auto& param_t = param.type_;
        auto converted = convert(std::move(args[i]), param_t);
        values_.define(param.name_.lexeme_, {param_t, std::move(converted)});
    }

    try {
        for (const auto& s : func->body_->statements_) execute(*s);
    } catch (const interpreter::return_exception& ret) { return ret.return_value_; }
    return default_value(func->return_type_);
}

core::value interpreter::evaluate_array_literal(const ast::array_literal_expr& expr) {
    std::vector<core::value> elements;
    elements.reserve(expr.elements_.size());
    std::ranges::transform(expr.elements_, std::back_inserter(elements),
                           [this](const auto& elem) { return evaluate(elem); });

    if (elements.empty()) return core::value(std::vector<core::value>{});

    auto elem_type = elements[0].type();
    for (auto& e : elements) e = convert(std::move(e), elem_type);

    return core::value(std::move(elements));
}

core::value interpreter::evaluate_index(const ast::index_expr& expr) {
    auto obj = evaluate(expr.object_);
    auto idx = evaluate(expr.index_);

    const auto* arr = obj.as<core::value::array_t>();
    auto i = *idx.as<core::value::int_t>();

    if (i < 0 || i >= static_cast<core::value::int_t>((*arr)->size()))
        reporter_.interpret_error(expr, err::index_out_of_bounds);

    return (*arr)->at(static_cast<size_t>(i));
}

core::value interpreter::default_value(const core::type& t) {
    switch (t.get_kind()) {
        case k::INT:
            return core::value{core::value::int_t{}};
        case k::DOUBLE:
            return core::value{core::value::double_t{}};
        case k::BOOL:
            return core::value{core::value::bool_t{}};
        case k::STRING:
            return core::value{std::string{}};
        case k::VOID:
            return core::value{};
        case k::ARRAY: {
            core::value::array_t elements(t.array_size(), default_value(t.element_type()));
            return core::value{std::move(elements)};
        }
        default:
            return core::value{};
    }
}

core::value interpreter::convert(core::value val, const core::type& target) {
    auto val_t = val.type();
    if (val_t == target) return val;

    if (target.is_int() && val.is_double())
        return core::value{static_cast<core::value::int_t>(*val.as<core::value::double_t>())};
    if (target.is_double() && val.is_int())
        return core::value{static_cast<core::value::double_t>(*val.as<core::value::int_t>())};

    return val;
}

}  // namespace runtime
