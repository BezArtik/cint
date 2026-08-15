// runtime/interpreter.cpp

#include "runtime/interpreter.hpp"

#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/error/error_codes.hpp"
#include "core/token/token_types.hpp"
#include "core/utils/builtins.hpp"
#include "core/utils/overloaded.hpp"
#include "core/utils/scoped_map.hpp"
#include "core/utils/symbol_registry.hpp"
#include "core/value/operations.hpp"
#include "core/value/value.hpp"
#include "debug/debug.hpp"
#include "debug/debug_writer.hpp"
#include "debug/trace_level.hpp"

#include <algorithm>
#include <array>
#include <iterator>
#include <memory_resource>
#include <string>
#include <utility>
#include <vector>

using tt = core::token_type;
using t = core::type;
using err = core::error_code;
namespace op = core::ops;

namespace {

core::value apply_increment(core::value& val, core::token_type op, bool return_old) {
    auto&& old_val = val;

    if (val.is_int()) {
        val = (op == tt::INCREMENT ? val.to_int() + 1 : val.to_int() - 1);
    } else if (val.is_double()) {
        val = (op == tt::INCREMENT ? val.to_double() + 1.0 : val.to_double() - 1.0);
    }

    return return_old ? old_val : val;
}

}  // namespace

struct interpreter::execution_result {
    enum class kind : uint8_t { normal, return_ };

    kind kind_{kind::normal};
    core::value value_;

    static execution_result normal() { return {kind::normal, {}}; }
    static execution_result return_(core::value v) { return {kind::return_, std::move(v)}; }

    bool is_return() const noexcept { return kind_ == kind::return_; }
};

interpreter::interpreter(core::error_reporter& reporter, const core::symbol_registry& registry,
                         const debug::debug_writer& writer)
    : reporter_{reporter}, registry_{registry}, writer_{writer} {}

void interpreter::interpret(std::span<const ast::statement> statements) {
    try {
        for (auto&& stmt : statements) {
            if (!std::holds_alternative<ast::node<ast::func_declaration>>(stmt)) execute(stmt);
        }
    } catch (const core::runtime_error&) {}
}
// clang-format off
interpreter::execution_result interpreter::execute(const ast::statement& stmt) {
    if (writer_.enabled(debug::trace_level::execution)) debug::print_statement(writer_, stmt);
    return core::visit(core::overloaded{
            [&](const ast::node<ast::expression_stmt>& s) { return execute_expression_stmt(*s); },
            [&](const ast::node<ast::var_declaration>& s) { return execute_var_declaration(*s); },
            [&](const ast::node<ast::block_stmt>& s) { return execute_block(*s); },
            [&](const ast::node<ast::while_stmt>& s) { return execute_while(*s); },
            [&](const ast::node<ast::for_stmt>& s) { return execute_for(*s); },
            [&](const ast::node<ast::if_stmt>& s) { return execute_if(*s); },
            [&](const ast::node<ast::return_stmt>& s) { return execute_return_stmt(*s); },
            [](const ast::node<ast::func_declaration>&) { return execution_result::normal(); },
            [](const ast::node<ast::struct_declaration>&) { return execution_result::normal(); }},
            stmt);
}
// clang-format on
interpreter::execution_result interpreter::execute_expression_stmt(const ast::expression_stmt& stmt) {
    evaluate(stmt.expr_);
    return execution_result::normal();
}

interpreter::execution_result interpreter::execute_var_declaration(const ast::var_declaration& stmt) {
    auto&& type = registry_.resolve_type(stmt.type_);
    auto&& name = stmt.name_.lexeme_;
    auto&& init_val = core::value::default_value(type);

    if (stmt.initializer_) {
        if (auto&& list = std::get_if<ast::node<ast::initializer_list_expr>>(&*stmt.initializer_)) {
            if (type.is_struct()) {
                auto&& st = init_val.as_mut<core::value::struct_t>();
                auto&& fields = st->type_.struct_fields();
                auto&& elems = list->get()->elements_;
                for (size_t i = 0; i < elems.size(); ++i) {
                    auto&& val = evaluate(elems[i]);
                    st->fields_[i] = core::value::convert(std::move(val), fields[i].second);
                }
            } else {
                init_val = evaluate_initializer_list(*list->get());
            }
        } else {
            auto&& init = evaluate(*stmt.initializer_);
            init_val = core::value::convert(std::move(init), type);
        }
    }

    values_.define(name, std::move(init_val));

    if (writer_.enabled(debug::trace_level::execution)) {
        auto&& var = values_.get(name);
        writer_.emit("  " + std::string(name) + " = " + var->to_string() + "\n");
    }
    return execution_result::normal();
}

interpreter::execution_result interpreter::execute_block(const ast::block_stmt& stmt, bool create_scope) {
    std::optional<core::scope_guard<core::value>> guard;
    if (create_scope) guard.emplace(values_);
    for (auto&& s : stmt.statements_) {
        auto&& res = execute(s);
        if (res.is_return()) return res;
    }
    return execution_result::normal();
}

interpreter::execution_result interpreter::execute_body(const ast::statement& body) {
    if (auto&& block = std::get_if<ast::node<ast::block_stmt>>(&body))
        return execute_block(**block, (*block)->has_declarations_);
    return execute(body);
}

interpreter::execution_result interpreter::execute_while(const ast::while_stmt& stmt) {
    core::scope_guard guard{values_};
    while (true) {
        auto&& cond = evaluate(stmt.condition_);
        if (!cond.to_bool()) break;

        auto&& res = execute_body(stmt.block_);
        if (res.is_return()) return res;
    }
    return execution_result::normal();
}

interpreter::execution_result interpreter::execute_for(const ast::for_stmt& stmt) {
    core::scope_guard guard{values_};
    if (stmt.initializer_) execute(*stmt.initializer_);
    while (true) {
        if (stmt.condition_) {
            auto&& cond = evaluate(*stmt.condition_);
            if (!cond.to_bool()) break;
        }
        auto&& result = execute_body(stmt.block_);
        if (result.is_return()) return result;

        if (stmt.increment_) evaluate(*stmt.increment_);
    }
    return execution_result::normal();
}

interpreter::execution_result interpreter::execute_if(const ast::if_stmt& stmt) {
    auto&& cond = evaluate(stmt.condition_);

    if (cond.to_bool()) {
        return execute_body(stmt.then_block_);
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

// clang-format off
core::value interpreter::evaluate(const ast::expression& expr) {
    auto&& result = core::visit(
        core::overloaded{
            [&](const ast::node<ast::literal_expr>& e) { return evaluate_literal(*e); },
            [&](const ast::node<ast::variable_expr>&) { return evaluate_lvalue(expr); },
            [&](const ast::node<ast::binary_expr>& e) { return evaluate_binary(*e); },
            [&](const ast::node<ast::assignment_expr>& e) { return evaluate_assignment(*e); },
            [&](const ast::node<ast::unary_expr>& e) { return evaluate_unary(*e); },
            [&](const ast::node<ast::postfix_expr>& e) { return evaluate_postfix(*e); },
            [&](const ast::node<ast::call_expr>& e) { return evaluate_call(*e); },
            [&](const ast::node<ast::initializer_list_expr>& e) { return evaluate_initializer_list(*e); },
            [&](const ast::node<ast::index_expr>&) { return evaluate_lvalue(expr); },
            [&](const ast::node<ast::member_access_expr>&) { return evaluate_lvalue(expr); }},
        expr);
    if (writer_.enabled(debug::trace_level::execution)) debug::print_expression(writer_, expr, 0, &result);
    return result;
}

core::value interpreter::evaluate_literal(const ast::literal_expr& expr) {
    return expr.value_;
}

core::value interpreter::evaluate_binary(const ast::binary_expr& expr) {
    auto&& left = evaluate(expr.left_);
    auto&& type = expr.op_.type_;

    if (type == tt::LOGICAL_AND) {
        if (!left.to_bool()) return false;
        return evaluate(expr.right_).to_bool();
    }
    if (type == tt::LOGICAL_OR) {
        if (left.to_bool()) return true;
        return evaluate(expr.right_).to_bool();
    }

    auto&& right = evaluate(expr.right_);
    try {
        switch (type) {
            case tt::PLUS:          return op::add(left, right);
            case tt::MINUS:         return op::sub(left, right);
            case tt::STAR:          return op::mul(left, right);
            case tt::SLASH:         return op::div(left, right);
            case tt::PERCENT:       return op::mod(left, right);
            case tt::EQUAL_EQUAL:   return op::eq(left, right);
            case tt::BANG_EQUAL:    return op::neq(left, right);
            case tt::LESS:          return op::lt(left, right);
            case tt::LESS_EQUAL:    return op::le(left, right);
            case tt::GREATER:       return op::gt(left, right);
            case tt::GREATER_EQUAL: return op::ge(left, right);
            case tt::BIT_AND:       return op::bit_and(left, right);
            case tt::BIT_OR:        return op::bit_or(left, right);
            case tt::XOR:           return op::bit_xor(left, right);
            case tt::SHL:           return op::shl(left, right);
            case tt::SHR:           return op::shr(left, right);
            default: reporter_.runtime_error(expr, err::unsupported_binary_operator, expr.op_.lexeme_);
        }
    } catch (const core::value_error& e) { reporter_.runtime_error(expr, e.code_, expr.op_.lexeme_); }
}


core::value& interpreter::evaluate_lvalue(const ast::expression& expr) {
    return core::visit(core::overloaded{
            [&](const ast::node<ast::variable_expr>& e) -> core::value& {                             
                auto&& var = values_.get(e->name_.lexeme_);                            
                return *var;
            },
            [&](const ast::node<ast::index_expr>& e) -> core::value& {
                auto&& obj = evaluate_lvalue(e->object_);
                auto&& index_val = evaluate(e->index_);
                auto&& i = index_val.to_int();
                auto&& arr = obj.as_mut<core::value::array_t>();

                if (i < 0 || i >= static_cast<core::value::int_t>((*arr)->size()))
                    throw core::runtime_error{err::index_out_of_bounds};

                return (*arr)->at(static_cast<size_t>(i));
            },
            [&](const ast::node<ast::member_access_expr>& e) -> core::value& {
                auto&& obj = evaluate_lvalue(e->object_);
                auto&& st = obj.as_mut<core::value::struct_t>();
                auto&& idx = st->type_.field_index(e->member_.lexeme_);
                return st->fields_[*idx];
            },
            [](const auto&) -> core::value& {
                throw core::runtime_error{err::type_mismatch_assignment};
            }
    }, expr);
}

core::value interpreter::evaluate_assignment(const ast::assignment_expr& expr) {
    auto&& op = expr.op_.type_;
    auto&& right = evaluate(expr.value_);
    auto&& target = evaluate_lvalue(expr.target_);

    if (op == tt::EQUAL) {
        target = core::value::convert(std::move(right), target.type());
        return target;
    }

    switch (op) {
        case tt::PLUS_EQUAL:    target = op::add(target, right);     break;
        case tt::MINUS_EQUAL:   target = op::sub(target, right);     break;
        case tt::STAR_EQUAL:    target = op::mul(target, right);     break;
        case tt::SLASH_EQUAL:   target = op::div(target, right);     break;
        case tt::PERCENT_EQUAL: target = op::mod(target, right);     break;
        case tt::BIT_AND_EQUAL: target = op::bit_and(target, right); break;
        case tt::BIT_OR_EQUAL:  target = op::bit_or(target, right);  break;
        case tt::XOR_EQUAL:     target = op::bit_xor(target, right); break;
        case tt::SHL_EQUAL:     target = op::shl(target, right);     break;
        case tt::SHR_EQUAL:     target = op::shr(target, right);     break;
        default: break;
    }

    return target;
}
// clang-format on

core::value interpreter::evaluate_unary(const ast::unary_expr& expr) {
    auto&& operand = evaluate(expr.operand_);
    auto&& op = expr.op_.type_;

    switch (op) {
        case tt::MINUS:
            return op::unary_minus(operand);
        case tt::BANG:
            return op::not_op(operand);
        case tt::BIT_NOT:
            return op::bit_not(operand);
        case tt::INCREMENT:
        case tt::DECREMENT:
            return apply_increment(evaluate_lvalue(expr.operand_), op, false);
        default:
            reporter_.runtime_error(expr, err::unsupported_unary_operator, expr.op_.lexeme_);
    }
}

core::value interpreter::evaluate_postfix(const ast::postfix_expr& expr) {
    return apply_increment(evaluate_lvalue(expr.operand_), expr.op_.type_, true);
}
// clang-format off
core::value interpreter::evaluate_call(const ast::call_expr& expr) {
    auto&& name = expr.callee_.lexeme_;

    if (recursion_depth_ >= MAX_RECURSION_DEPTH) reporter_.runtime_error(expr, err::stack_overflow);
    recursion_depth_++;

    struct depth_guard {
        uint32_t& d;
        ~depth_guard() { d--; }
    } d_guard{recursion_depth_};

    std::array<std::byte, 4096> args_buf;
    std::pmr::monotonic_buffer_resource args_mr{args_buf.data(), args_buf.size()};
    std::pmr::vector<core::value> args_vec(&args_mr);
    try {
        std::ranges::transform(expr.args_, std::back_inserter(args_vec),
                               [&](auto&& arg) { return evaluate(arg); });
    } catch (const core::runtime_error&) { return {}; }

    auto&& func = registry_.find(name);
    std::span<const core::value> args{args_vec.data(), expr.args_.size()};

    return core::visit(
        core::overloaded{
        [&](core::builtin_fn_ptr builtin) -> core::value {
            try {
                auto&& r = builtin(args);
                if (writer_.enabled(debug::trace_level::returns)) debug::print_return(writer_, name, r);
                return r;
            } catch (const core::value_error& e) {
                reporter_.runtime_error(expr, e.code_, name);
                return {};
            }
        },
        [&](core::symbol_registry::func_ptr body) -> core::value {
            core::scope_guard guard{values_};
            for (size_t i = 0; i < body->params_.size(); ++i) {
                auto&& param = body->params_[i];
                auto&& converted = core::value::convert(std::move(args_vec[i]), registry_.resolve_type(param.type_));
                values_.define(param.name_.lexeme_, std::move(converted));
            }
            auto&& block = std::get<ast::node<ast::block_stmt>>(body->block_);
            for (auto&& s : block->statements_) {
                auto&& result = execute(s);
                if (result.is_return()) {
                    if (writer_.enabled(debug::trace_level::returns)) 
                        debug::print_return(writer_, name, result.value_);
                    return result.value_; 
                }
            }
            auto&& r = core::value::default_value(body->return_type_);
            if (writer_.enabled(debug::trace_level::returns)) debug::print_return(writer_, name, r);
            return r;
        },
        [&](core::symbol_registry::struct_ptr) -> core::value {
            reporter_.runtime_error(expr, err::not_a_function, name);
            return {};
        }},
        func->info_);
}
// clang-format on
core::value interpreter::evaluate_initializer_list(const ast::initializer_list_expr& expr) {
    std::vector<core::value> elements;
    elements.reserve(expr.elements_.size());
    std::ranges::transform(expr.elements_, std::back_inserter(elements), [&](auto&& elem) { return evaluate(elem); });

    if (elements.empty()) return std::vector<core::value>{};

    auto&& elem_type = elements[0].type();
    for (auto&& e : elements) e = core::value::convert(std::move(e), elem_type);

    return elements;
}
