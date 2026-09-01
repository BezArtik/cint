// runtime/interpreter.cpp

#include "runtime/interpreter.hpp"

#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/builtins/builtins.hpp"
#include "core/error/error_codes.hpp"
#include "core/symbol/symbol_registry.hpp"
#include "core/token/token_types.hpp"
#include "core/utils/overloaded.hpp"
#include "core/utils/scoped_map.hpp"
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
    auto old_val = val;

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

void interpreter::interpret(std::span<const ast::statement> statements) {
    try {
        for (auto&& stmt : statements) {
            if (!stmt.holds<ast::func_declaration_stmt>()) execute(stmt);
        }
    } catch (const core::runtime_error&) {}
}
// clang-format off
interpreter::execution_result interpreter::execute(const ast::statement& stmt) {
    if (writer_.enabled(debug::trace_level::execution)) debug::print_statement(writer_, stmt);
    return stmt.visit(core::overloaded{
            [&](const ast::expression_stmt& s) { return execute_expression_stmt(s); },
            [&](const ast::var_declaration_stmt& s) { return execute_var_declaration(s); },
            [&](const ast::block_stmt& s) { return execute_block(s); },
            [&](const ast::while_stmt& s) { return execute_while(s); },
            [&](const ast::for_stmt& s) { return execute_for(s); },
            [&](const ast::if_stmt& s) { return execute_if(s); },
            [&](const ast::return_stmt& s) { return execute_return_stmt(s); },
            [](const ast::func_declaration_stmt&) { return execution_result::normal(); },
            [](const ast::struct_declaration_stmt&) { return execution_result::normal(); }
            });
}
// clang-format on
interpreter::execution_result interpreter::execute_expression_stmt(const ast::expression_stmt& stmt) {
    evaluate(stmt.expr_);
    return execution_result::normal();
}

interpreter::execution_result interpreter::execute_var_declaration(const ast::var_declaration_stmt& stmt) {
    auto&& type = registry_.resolve_type(stmt.type_);
    auto&& name = stmt.name_.lexeme_;
    auto&& init_val = core::value::default_value(type);

    if (stmt.initializer_) {
        if (auto&& list = stmt.initializer_->get_if<ast::initializer_list_expr>()) {
            if (type.is_struct()) {
                auto&& st = init_val.to_struct();
                std::ranges::transform(list->elements_, st.fields_.begin(),
                                       [&](auto&& elem) { return evaluate(elem); });
            } else {
                init_val = evaluate_initializer_list(*list);
            }
        } else {
            init_val = evaluate(*stmt.initializer_);
        }
    }

    values_.define(name, init_val);

    if (writer_.enabled(debug::trace_level::execution)) {
        auto&& var = values_.get(name);
        writer_.emit("  " + std::string{name} + " = " + var->to_string() + "\n");
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
    if (auto&& block = body.get_if<ast::block_stmt>()) return execute_block(*block, block->has_declarations_);
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
    auto&& result = expr.visit(
        core::overloaded{
            [&](const ast::literal_expr& e) { return evaluate_literal(e); },
            [&](const ast::variable_expr&) { return evaluate_lvalue(expr); },
            [&](const ast::binary_expr& e) { return evaluate_binary(e); },
            [&](const ast::assignment_expr& e) { return evaluate_assignment(e); },
            [&](const ast::unary_expr& e) { return evaluate_unary(e); },
            [&](const ast::postfix_expr& e) { return evaluate_postfix(e); },
            [&](const ast::call_expr& e) { return evaluate_call(e); },
            [&](const ast::initializer_list_expr& e) { return evaluate_initializer_list(e); },
            [&](const ast::index_expr&) { return evaluate_lvalue(expr); },
            [&](const ast::member_access_expr&) { return evaluate_lvalue(expr); }
            });
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
            default: reporter_.runtime_error(expr.op_.loc_, err::unsupported_binary_operator, expr.op_.lexeme_);
        }
    } catch (const core::value_error& e) { reporter_.runtime_error(expr.op_.loc_, e.code_, expr.op_.lexeme_); }
}


core::value& interpreter::evaluate_lvalue(const ast::expression& expr) {
    return expr.visit(core::overloaded{
            [&](const ast::variable_expr& e) -> core::value& {                             
                auto&& var = values_.get(e.name_.lexeme_);                            
                return *var;
            },
            [&](const ast::index_expr& e) -> core::value& {
                auto&& obj = evaluate_lvalue(e.object_);
                auto&& index_val = evaluate(e.index_);
                auto&& i = index_val.to_int();
                auto&& arr = obj.to_array();

                if (i < 0 || i >= static_cast<core::value::int_t>(arr.size()))
                    reporter_.runtime_error(e.loc_, err::index_out_of_bounds);

                return arr[i];
            },
            [&](const ast::member_access_expr& e) -> core::value& {
                auto&& obj = evaluate_lvalue(e.object_);
                auto&& st = obj.to_struct();
                auto&& idx = st.type_.field_index(e.member_.lexeme_);
                return st.fields_[*idx];
            },
            [](const auto&) -> core::value& {
                throw core::runtime_error{err::type_mismatch_assignment};
            }
    });
}

core::value interpreter::evaluate_assignment(const ast::assignment_expr& expr) {
    auto&& op = expr.op_.type_;
    auto&& right = evaluate(expr.value_);
    auto&& target = evaluate_lvalue(expr.target_);

    if (op == tt::EQUAL) {
        target = std::move(right);
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
            reporter_.runtime_error(expr.op_.loc_, err::unsupported_unary_operator, expr.op_.lexeme_);
    }
}

core::value interpreter::evaluate_postfix(const ast::postfix_expr& expr) {
    return apply_increment(evaluate_lvalue(expr.operand_), expr.op_.type_, true);
}
// clang-format off
core::value interpreter::evaluate_call(const ast::call_expr& expr) {
    auto&& name = expr.callee_.lexeme_;

    if (recursion_depth_ >= MAX_RECURSION_DEPTH) reporter_.runtime_error(expr.callee_.loc_, err::stack_overflow);
    recursion_depth_++;

    struct depth_guard {
        uint32_t& d;
        ~depth_guard() { d--; }
    } d_guard{recursion_depth_};

    std::array<std::byte, 512> args_buf;
    std::pmr::monotonic_buffer_resource args_mr{args_buf.data(), args_buf.size()};
    std::pmr::vector<core::value> args_vec(&args_mr);

    std::ranges::transform(expr.args_, std::back_inserter(args_vec),
            [&](auto&& arg) { return evaluate(arg); });

    auto&& func = registry_.find(name);
    return core::visit(
        core::overloaded{
        [&](core::symbol_registry::builtin_func_ptr builtin) -> core::value {
            try {
                auto&& r = builtin({args_vec.data(), args_vec.size()});
                if (writer_.enabled(debug::trace_level::returns)) debug::print_return(writer_, name, r);
                return r;
            } catch (const core::value_error& e) {
                reporter_.runtime_error(expr.callee_.loc_, e.code_, name);
                return {};
            }
        },
        [&](core::symbol_registry::func_ptr body) -> core::value {
            core::scope_guard guard{values_};
            auto&& params = body->type_.param_infos();
            for (size_t i = 0; i < params.size(); ++i) values_.define(params[i].first, args_vec[i]);
            auto&& block = body->block_.get<ast::block_stmt>();
            for (auto&& s : block.statements_) {
                auto&& result = execute(s);
                if (result.is_return()) {
                    if (writer_.enabled(debug::trace_level::returns)) 
                        debug::print_return(writer_, name, result.value_);
                    return result.value_; 
                }
            }
            auto&& r = core::value::default_value(body->type_.return_type());
            if (writer_.enabled(debug::trace_level::returns)) debug::print_return(writer_, name, r);
            return r;
        },
        [&](core::symbol_registry::struct_ptr) -> core::value {
            reporter_.runtime_error(expr.callee_.loc_, err::not_a_function, name);
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

    return elements;
}
