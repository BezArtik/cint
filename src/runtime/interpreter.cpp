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
#include "debug/debug_writer.hpp"

#include <algorithm>
#include <array>
#include <iterator>
#include <ranges>
#include <utility>
#include <vector>

namespace cint {

namespace {

struct runtime_error : std::exception {
    const char* what() const noexcept override { return "Runtime error"; }
};

template <typename... Args>
[[noreturn]] void raise_error(core::diagnostic_sink& sink, core::location loc, core::error_code code, Args&&... args) {
    sink.report(loc, code, std::forward<Args>(args)...);
    throw runtime_error{};
}

}  // namespace

using tt = core::token_type;
using t = core::type;
using err = core::error_code;

struct interpreter::execution_result {
    enum class kind : std::uint8_t { normal, return_ };

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
    } catch (const runtime_error&) {}
}

interpreter::execution_result interpreter::execute(const ast::statement& stmt) {
    if (writer_.enabled(debug::trace_level::execution)) [[unlikely]]
        writer_.print(stmt);
    return stmt.visit([&](const auto& s) { return execute(s); });
}

interpreter::execution_result interpreter::execute(const ast::expression_stmt& stmt) {
    std::ignore = evaluate(stmt.expr_);
    return execution_result::normal();
}

interpreter::execution_result interpreter::execute(const ast::var_declaration_stmt& stmt) {
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
                init_val = evaluate(*list);
            }
        } else {
            init_val = evaluate(*stmt.initializer_);
        }
    }

    values_.define(name, init_val);

    if (writer_.enabled(debug::trace_level::execution)) [[unlikely]] {
        auto&& var = values_.get(name);
        writer_.print_var(name, *var);
    }
    return execution_result::normal();
}

interpreter::execution_result interpreter::execute(const ast::func_declaration_stmt&) {
    return execution_result::normal();
}

interpreter::execution_result interpreter::execute(const ast::struct_declaration_stmt&) {
    return execution_result::normal();
}

interpreter::execution_result interpreter::execute(const ast::block_stmt& stmt) {
    auto guard = std::optional<core::scope_guard<core::value>>{};
    if (stmt.has_declarations_) guard.emplace(values_);
    for (auto&& s : stmt.statements_) {
        auto&& res = execute(s);
        if (res.is_return()) return res;
    }
    return execution_result::normal();
}

interpreter::execution_result interpreter::execute(const ast::while_stmt& stmt) {
    auto guard = core::scope_guard{values_};
    while (true) {
        auto&& cond = evaluate(stmt.condition_);
        if (!cond.to_bool()) break;

        auto&& res = execute(stmt.block_);
        if (res.is_return()) return res;
    }
    return execution_result::normal();
}

interpreter::execution_result interpreter::execute(const ast::for_stmt& stmt) {
    auto guard = core::scope_guard{values_};
    if (stmt.initializer_) execute(*stmt.initializer_);
    while (true) {
        if (stmt.condition_) {
            auto&& cond = evaluate(*stmt.condition_);
            if (!cond.to_bool()) break;
        }
        auto&& result = execute(stmt.block_);
        if (result.is_return()) return result;

        if (stmt.increment_) std::ignore = evaluate(*stmt.increment_);
    }
    return execution_result::normal();
}

interpreter::execution_result interpreter::execute(const ast::if_stmt& stmt) {
    auto&& cond = evaluate(stmt.condition_);

    if (cond.to_bool()) {
        return execute(stmt.then_block_);
    } else if (stmt.else_block_) {
        return execute(*stmt.else_block_);
    }
    return execution_result::normal();
}

interpreter::execution_result interpreter::execute(const ast::return_stmt& stmt) {
    auto ret_val = core::value{};
    if (stmt.value_) ret_val = evaluate(*stmt.value_);
    return execution_result::return_(ret_val);
}

core::value interpreter::evaluate(const ast::expression& expr) {
    auto&& result = expr.visit([&](const auto& e) { return evaluate(e); });
    if (writer_.enabled(debug::trace_level::execution)) [[unlikely]]
        writer_.print(expr, &result);
    return result;
}

core::value interpreter::evaluate(const ast::literal_expr& expr) {
    return expr.value_;
}

core::value& interpreter::evaluate_lvalue(const ast::expression& expr) {
    return expr.visit(
        core::overloaded{[&](const ast::variable_expr& e) -> core::value& { return evaluate_lvalue(e); },
                         [&](const ast::index_expr& e) -> core::value& { return evaluate_lvalue(e); },
                         [&](const ast::member_access_expr& e) -> core::value& { return evaluate_lvalue(e); },
                         [](const auto&) -> core::value& { throw runtime_error{}; }});
}

core::value& interpreter::evaluate_lvalue(const ast::variable_expr& expr) {
    return *values_.get(expr.name_.lexeme_);
}

core::value& interpreter::evaluate_lvalue(const ast::index_expr& expr) {
    auto&& obj = evaluate_lvalue(expr.object_);
    auto&& index_val = evaluate(expr.index_);
    auto&& i = index_val.to_int();
    auto&& arr = obj.to_array();

    if (i < 0 || i >= static_cast<core::value::int_t>(arr.size())) [[unlikely]]
        raise_error(sink_, expr.loc_, err::index_out_of_bounds);

    return arr[i];
}

core::value& interpreter::evaluate_lvalue(const ast::member_access_expr& expr) {
    auto&& obj = evaluate_lvalue(expr.object_);
    auto&& st = obj.to_struct();
    auto&& idx = st.type_.field_index(expr.member_.lexeme_);
    return st.fields_[*idx];
}

core::value interpreter::evaluate(const ast::variable_expr& expr) {
    return evaluate_lvalue(expr);
}
core::value interpreter::evaluate(const ast::member_access_expr& expr) {
    return evaluate_lvalue(expr);
}
core::value interpreter::evaluate(const ast::index_expr& expr) {
    return evaluate_lvalue(expr);
}
// clang-format off
core::value interpreter::evaluate(const ast::binary_expr& expr) {
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
            case tt::PLUS:          return core::add(left, right);
            case tt::MINUS:         return core::sub(left, right);
            case tt::STAR:          return core::mul(left, right);
            case tt::SLASH:         return core::div(left, right);
            case tt::PERCENT:       return core::mod(left, right);
            case tt::EQUAL_EQUAL:   return core::eq(left, right);
            case tt::BANG_EQUAL:    return core::neq(left, right);
            case tt::LESS:          return core::lt(left, right);
            case tt::LESS_EQUAL:    return core::le(left, right);
            case tt::GREATER:       return core::gt(left, right);
            case tt::GREATER_EQUAL: return core::ge(left, right);
            case tt::BIT_AND:       return core::bit_and(left, right);
            case tt::BIT_OR:        return core::bit_or(left, right);
            case tt::XOR:           return core::bit_xor(left, right);
            case tt::SHL:           return core::shl(left, right);
            case tt::SHR:           return core::shr(left, right);
            default:                return {};
        }
    } catch (const core::value_error& e) { raise_error(sink_, expr.op_.loc_, e.code_, expr.op_.lexeme_); }
}

core::value interpreter::evaluate(const ast::assignment_expr& expr) {
    auto&& op = expr.op_.type_;
    auto&& right = evaluate(expr.value_);
    auto&& target = evaluate_lvalue(expr.target_);

    switch (op) {
        case tt::EQUAL:         target = std::move(right);             break;
        case tt::PLUS_EQUAL:    target = core::add(target, right);     break;
        case tt::MINUS_EQUAL:   target = core::sub(target, right);     break;
        case tt::STAR_EQUAL:    target = core::mul(target, right);     break;
        case tt::SLASH_EQUAL:   target = core::div(target, right);     break;
        case tt::PERCENT_EQUAL: target = core::mod(target, right);     break;
        case tt::BIT_AND_EQUAL: target = core::bit_and(target, right); break;
        case tt::BIT_OR_EQUAL:  target = core::bit_or(target, right);  break;
        case tt::XOR_EQUAL:     target = core::bit_xor(target, right); break;
        case tt::SHL_EQUAL:     target = core::shl(target, right);     break;
        case tt::SHR_EQUAL:     target = core::shr(target, right);     break;
        default: break;
    }

    return target;
}
// clang-format on

core::value interpreter::evaluate(const ast::unary_expr& expr) {
    auto&& operand = evaluate(expr.operand_);
    auto&& op = expr.op_.type_;

    switch (op) {
        case tt::MINUS:
            return core::unary_minus(operand);
        case tt::BANG:
            return core::not_op(operand);
        case tt::BIT_NOT:
            return core::bit_not(operand);
        case tt::INCREMENT:
            return core::pre_increment(evaluate_lvalue(expr.operand_));
        case tt::DECREMENT:
            return core::pre_decrement(evaluate_lvalue(expr.operand_));

        default:
            return {};
    }
}

core::value interpreter::evaluate(const ast::postfix_expr& expr) {
    return expr.op_.type_ == tt::INCREMENT ? core::post_increment(evaluate_lvalue(expr.operand_))
                                           : core::post_decrement(evaluate_lvalue(expr.operand_));
}

core::value interpreter::evaluate(const ast::initializer_list_expr& expr) {
    auto elements = std::vector<core::value>{};
    elements.reserve(expr.elements_.size());
    std::ranges::transform(expr.elements_, std::back_inserter(elements), [&](auto&& elem) { return evaluate(elem); });
    return elements;
}

core::value interpreter::evaluate(const ast::call_expr& expr) {
    auto&& name = expr.callee_.lexeme_;

    if (recursion_depth_ >= MAX_RECURSION_DEPTH) [[unlikely]]
        raise_error(sink_, expr.callee_.loc_, err::stack_overflow);
    recursion_depth_++;

    struct depth_guard {
        uint32_t& d;
        ~depth_guard() { d--; }
    } d_guard{recursion_depth_};

    auto args_buf = std::array<std::byte, 512>{};
    auto args_mr = std::pmr::monotonic_buffer_resource{args_buf.data(), args_buf.size()};
    auto args_vec = std::pmr::vector<core::value>(&args_mr);

    std::ranges::transform(expr.args_, std::back_inserter(args_vec), [&](auto&& arg) { return evaluate(arg); });

    auto&& func = registry_.find(name);
    try {
        return func->info_.visit([&](const auto& ptr) { return call(ptr, args_vec, name); });
    } catch (const core::value_error& e) { raise_error(sink_, expr.callee_.loc_, e.code_, name); }
}

core::value interpreter::call(core::symbol_registry::builtin_func_ptr builtin, std::span<const core::value> args,
                              std::string_view name) {
    auto&& result = builtin->impl_(args);
    if (writer_.enabled(debug::trace_level::returns)) [[unlikely]]
        writer_.print_return(name, result);
    return result;
}

core::value interpreter::call(core::symbol_registry::func_ptr body, std::span<const core::value> args,
                              std::string_view name) {
    auto guard = core::scope_guard{values_};
    auto&& params = body->type_.param_infos();
    for (auto&& [param, arg] : std::views::zip(params, args)) values_.define(param.first, arg);

    auto&& block = body->block_.get<ast::block_stmt>();
    for (auto&& s : block.statements_) {
        auto&& result = execute(s);
        if (result.is_return()) {
            if (writer_.enabled(debug::trace_level::returns)) [[unlikely]]
                writer_.print_return(name, result.value_);
            return result.value_;
        }
    }

    auto&& result = core::value::default_value(body->type_.return_type());
    if (writer_.enabled(debug::trace_level::returns)) [[unlikely]]
        writer_.print_return(name, result);
    return result;
}

core::value interpreter::call(core::symbol_registry::struct_ptr, std::span<const core::value>, std::string_view) {
    throw runtime_error{};
}

}  // namespace cint
