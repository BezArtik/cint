// runtime/interpreter.cpp

#include "runtime/interpreter.hpp"

#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/error/error_codes.hpp"
#include "core/token/token_types.hpp"
#include "core/utils/arena.hpp"
#include "core/utils/builtins.hpp"
#include "core/utils/function_registry.hpp"
#include "core/utils/overloaded.hpp"
#include "core/utils/scoped_map.hpp"
#include "core/value/operations.hpp"
#include "core/value/value.hpp"
#include "debug/debug.hpp"
#include "debug/debug_writer.hpp"
#include "debug/trace_level.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <string>
#include <utility>

namespace runtime {

using tt = core::token_type;
using t = core::type;
using err = core::error_code;
namespace op = core::ops;

namespace {

core::value apply_increment(core::value& val, core::token_type op, bool return_old) {
    auto old_val = val;

    if (auto* i = val.as_mut<core::value::int_t>()) {
        val = (op == tt::INCREMENT ? *i + 1 : *i - 1);
    } else if (auto* d = val.as_mut<core::value::double_t>()) {
        val = (op == tt::INCREMENT ? *d + 1.0 : *d - 1.0);
    }

    return return_old ? old_val : val;
}

}  // namespace

struct interpreter::execution_result {
    enum class kind : uint8_t { normal, return_ };

    kind kind_ = kind::normal;
    core::value value_;

    static execution_result normal() { return {kind::normal, {}}; }
    static execution_result return_(core::value v) { return {kind::return_, std::move(v)}; }

    bool is_normal() const noexcept { return kind_ == kind::normal; }
    bool is_return() const noexcept { return kind_ == kind::return_; }
};

interpreter::interpreter(core::error_reporter& reporter, const core::function_registry& registry,
                         const debug::debug_writer& writer)
    : reporter_(reporter), registry_(registry), writer_(writer) {}

void interpreter::interpret(const std::vector<ast::stmt_ptr>& statements) {
    try {
        for (const auto& stmt : statements) {
            if (!std::holds_alternative<ast::func_declaration>(stmt->data_)) execute(*stmt);
        }
    } catch (const core::interpret_error&) {}
}

interpreter::execution_result interpreter::execute(const ast::statement& stmt) {
    if (writer_.enabled(debug::trace_level::execution)) debug::print_statement(writer_, stmt);
    return core::visit(core::overloaded{
                           [this](const ast::expression_stmt& s) { return execute_expression_stmt(s); },
                           [this](const ast::var_declaration& s) { return execute_var_declaration(s); },
                           [this](const ast::block_stmt& s) { return execute_block(s); },
                           [this](const ast::while_stmt& s) { return execute_while(s); },
                           [this](const ast::for_stmt& s) { return execute_for(s); },
                           [this](const ast::if_stmt& s) { return execute_if(s); },
                           [this](const ast::return_stmt& s) { return execute_return_stmt(s); },
                           [](const ast::func_declaration&) { return execution_result::normal(); },
                       },
                       stmt.data_);
}

interpreter::execution_result interpreter::execute_expression_stmt(const ast::expression_stmt& stmt) {
    evaluate(stmt.expr_);
    return execution_result::normal();
}

interpreter::execution_result interpreter::execute_var_declaration(const ast::var_declaration& stmt) {
    auto init_val = core::value::default_value(stmt.type_);

    if (stmt.initializer_) {
        auto init = evaluate(*stmt.initializer_);
        init_val = core::value::convert(std::move(init), stmt.type_);
    }

    values_.define(stmt.name_.lexeme_, {&stmt.type_, std::move(init_val)});

    if (writer_.enabled(debug::trace_level::execution)) {
        auto* var = values_.get(stmt.name_.lexeme_);
        writer_.emit("  " + std::string(stmt.name_.lexeme_) + " = " + var->value_.to_string() + "\n");
    }
    return execution_result::normal();
}

interpreter::execution_result interpreter::execute_block(const ast::block_stmt& stmt, bool create_scope) {
    std::optional<core::scope_guard<runtime_var>> guard;
    if (create_scope) guard.emplace(values_);
    for (const auto& s : stmt.statements_) {
        auto res = execute(*s);
        if (res.is_return()) return res;
    }
    return execution_result::normal();
}

interpreter::execution_result interpreter::execute_body(const ast::statement& body) {
    bool create_scope = false;
    if (auto* block = std::get_if<ast::block_stmt>(&body.data_)) {
        create_scope = ast::has_declarations(*block);
        return execute_block(*block, create_scope);
    }
    return execute(body);
}

interpreter::execution_result interpreter::execute_while(const ast::while_stmt& stmt) {
    core::scope_guard guard(values_);
    while (true) {
        auto cond = evaluate(stmt.condition_);
        if (!cond.to_bool()) break;

        auto res = execute_body(*stmt.block_);
        if (res.is_return()) return res;
    }
    return execution_result::normal();
}

interpreter::execution_result interpreter::execute_for(const ast::for_stmt& stmt) {
    core::scope_guard guard(values_);
    if (stmt.initializer_) execute(*stmt.initializer_);
    while (true) {
        if (stmt.condition_) {
            auto cond = evaluate(*stmt.condition_);
            if (!cond.to_bool()) break;
        }
        auto result = execute_body(*stmt.block_);
        if (result.is_return()) return result;

        if (stmt.increment_) evaluate(*stmt.increment_);
    }
    return execution_result::normal();
}

interpreter::execution_result interpreter::execute_if(const ast::if_stmt& stmt) {
    auto cond = evaluate(stmt.condition_);

    if (cond.to_bool()) {
        return execute_body(*stmt.then_block_);
    } else if (stmt.else_block_) {
        return execute_body(*stmt.else_block_);
    }
    return execution_result::normal();
}

interpreter::execution_result interpreter::execute_return_stmt(const ast::return_stmt& stmt) {
    core::value ret_val;
    if (stmt.value_) ret_val = evaluate(*stmt.value_);
    return execution_result::return_(ret_val);
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
    if (writer_.enabled(debug::trace_level::execution)) debug::print_expression(writer_, expr, 0, &result);
    return result;
}

core::value interpreter::evaluate_literal(const ast::literal_expr& expr) {
    const auto& token = expr.value_;

    if (token.literal_value_) return *token.literal_value_;

    if (token.type_ == tt::STRING) {
        auto lex = token.lexeme_;
        return std::string(lex.substr(1, lex.size() - 2));
    }

    reporter_.interpret_error(token, err::unexpected_literal);
    return {};
}

core::value interpreter::evaluate_variable(const ast::variable_expr& expr) {
    return evaluate_lvalue(expr);
}

core::value interpreter::evaluate_binary(const ast::binary_expr& expr) {
    auto left = evaluate(expr.left_);
    auto type = expr.op_.type_;

    if (type == tt::LOGICAL_AND) {
        if (!left.to_bool()) return false;
        return evaluate(expr.right_).to_bool();
    }
    if (type == tt::LOGICAL_OR) {
        if (left.to_bool()) return true;
        return evaluate(expr.right_).to_bool();
    }

    auto right = evaluate(expr.right_);
    try {
        switch (type) {
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

// clang-format off
core::value& interpreter::evaluate_lvalue(const ast::expression& expr) {
    return core::visit(core::overloaded{
            [this](const ast::variable_expr& e) -> core::value& {                             
                auto* var = values_.get(e.name_.lexeme_);                            
                return var->value_;
            },
            [this](const core::arena_ptr<ast::index_expr>& e) -> core::value& {
                auto& obj = evaluate_lvalue(e->object_);
                auto index_val = evaluate(e->index_);
                auto i = *index_val.as<core::value::int_t>();

                auto* arr = obj.as_mut<core::value::array_t>();
                if (i < 0 || i >= static_cast<core::value::int_t>((*arr)->size()))
                    throw core::interpret_error{err::index_out_of_bounds};

                return (*arr)->at(static_cast<size_t>(i));
            },
            [](const auto&) -> core::value& {
                throw core::interpret_error{err::type_mismatch_assignment};
            }
    }, expr);
}
// clang-format on

core::value interpreter::evaluate_assignment(const ast::assignment_expr& expr) {
    auto op = expr.op_.type_;
    auto right = evaluate(expr.value_);

    auto& target = evaluate_lvalue(expr.target_);

    if (op == tt::EQUAL) {
        target = core::value::convert(std::move(right), target.type());
        return target;
    }

    switch (op) {
        case tt::PLUS_EQUAL:
            target = op::add(target, right);
            break;
        case tt::MINUS_EQUAL:
            target = op::sub(target, right);
            break;
        case tt::STAR_EQUAL:
            target = op::mul(target, right);
            break;
        case tt::SLASH_EQUAL:
            target = op::div(target, right);
            break;
        case tt::PERCENT_EQUAL:
            target = op::mod(target, right);
            break;
        case tt::BIT_AND_EQUAL:
            target = op::bit_and(target, right);
            break;
        case tt::BIT_OR_EQUAL:
            target = op::bit_or(target, right);
            break;
        case tt::XOR_EQUAL:
            target = op::bit_xor(target, right);
            break;
        case tt::SHL_EQUAL:
            target = op::shl(target, right);
            break;
        case tt::SHR_EQUAL:
            target = op::shr(target, right);
            break;
        default:
            break;
    }

    return target;
}

core::value interpreter::evaluate_unary(const ast::unary_expr& expr) {
    auto operand = evaluate(expr.operand_);
    auto op = expr.op_.type_;

    switch (op) {
        case tt::MINUS: {
            if (auto* i = operand.as<core::value::int_t>()) return -*i;
            return -operand.to_double();
        }
        case tt::BANG:
            return op::not_op(operand);
        case tt::BIT_NOT:
            return op::bit_not(operand);
        case tt::INCREMENT:
        case tt::DECREMENT:
            return apply_increment(evaluate_lvalue(expr.operand_), op, false);
        default:
            reporter_.interpret_error(expr, err::unsupported_unary_operator, expr.op_.lexeme_);
    }
}

core::value interpreter::evaluate_postfix(const ast::postfix_expr& expr) {
    return apply_increment(evaluate_lvalue(expr.operand_), expr.op_.type_, true);
}

core::value interpreter::evaluate_call(const ast::call_expr& expr) {
    auto name = expr.callee_.lexeme_;

    if (recursion_depth_ >= MAX_RECURSION_DEPTH) reporter_.interpret_error(expr, err::stack_overflow);
    recursion_depth_++;

    struct depth_guard {
        uint32_t& d;
        ~depth_guard() { d--; }
    } d_guard{recursion_depth_};

    std::array<core::value, MAX_ARGUMENTS> args_buf;
    try {
        std::ranges::transform(expr.args_, args_buf.begin(), [this](const auto& arg) { return evaluate(arg); });
    } catch (const core::interpret_error&) { return {}; }

    auto func = registry_.find(name);
    std::span<const core::value> args(args_buf.data(), expr.args_.size());

    if (func->builtin_) {
        try {
            auto r = func->builtin_(args);
            if (writer_.enabled(debug::trace_level::returns)) debug::print_return(writer_, name, r);
            return r;
        } catch (const core::interpret_error& e) {
            reporter_.interpret_error(expr, e.code_, name);
            return {};
        }
    }

    if (func->body_) {
        core::scope_guard guard(values_);
        for (size_t i = 0; i < func->body_->params_.size(); ++i) {
            const auto& param = func->body_->params_[i];
            auto converted = core::value::convert(std::move(args_buf[i]), param.type_);
            values_.define(param.name_.lexeme_, {&param.type_, std::move(converted)});
        }
        for (const auto& s : func->body_->block_->statements_) {
            auto result = execute(*s);
            if (result.is_return()) {
                if (writer_.enabled(debug::trace_level::returns)) debug::print_return(writer_, name, result.value_);
                return result.value_;
            }
        }
        auto r = core::value::default_value(func->body_->return_type_);
        if (writer_.enabled(debug::trace_level::returns)) debug::print_return(writer_, name, r);
        return r;
    }

    reporter_.interpret_error(expr, err::undefined_function, name);
}

core::value interpreter::evaluate_array_literal(const ast::array_literal_expr& expr) {
    std::vector<core::value> elements;
    elements.reserve(expr.elements_.size());
    std::ranges::transform(expr.elements_, std::back_inserter(elements),
                           [this](const auto& elem) { return evaluate(elem); });

    if (elements.empty()) return std::vector<core::value>{};

    auto elem_type = elements[0].type();
    for (auto& e : elements) e = core::value::convert(std::move(e), elem_type);

    return elements;
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

}  // namespace runtime
