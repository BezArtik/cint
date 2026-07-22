// semantics/type_check.cpp

#include "semantics/type_check.hpp"

#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/error/error_codes.hpp"
#include "core/token/token_types.hpp"
#include "core/utils/arena.hpp"
#include "core/utils/overloaded.hpp"
#include "core/utils/scoped_map.hpp"
#include "core/utils/symbol_registry.hpp"

#include <unordered_set>

namespace semantics {

using tt = core::token_type;
using t = core::type;
using err = core::error_code;

type_checker::type_checker(core::error_reporter& reporter, const core::symbol_registry& registry)
    : reporter_(reporter), registry_(registry) {}

bool type_checker::check(std::span<const ast::stmt_ptr> statements) {
    for (const auto& stmt : statements) check_statement(*stmt);
    return !reporter_.has_error();
}
// clang-format off
void type_checker::check_statement(const ast::statement& stmt) {
    core::visit(core::overloaded{
            [this](const ast::expression_stmt& s) { check_expression_stmt(s); },
            [this](const ast::var_declaration& s) { check_var_declaration(s); },
            [this](const ast::block_stmt& s) { check_block(s); },
            [this](const ast::while_stmt& s) { check_while(s); },
            [this](const ast::for_stmt& s) { check_for(s); },
            [this](const ast::if_stmt& s) { check_if(s); },
            [this](const ast::return_stmt& s) { check_return_stmt(s); },
            [this](const ast::func_declaration& s) { check_func_declaration(s); },
            [this](const ast::struct_declaration& s) { check_struct_declaration(s); }},
        stmt.data_);
}
// clang-format on
void type_checker::check_expression_stmt(const ast::expression_stmt& stmt) {
    type_of(stmt.expr_);
}

void type_checker::check_var_declaration(const ast::var_declaration& stmt) {
    auto name = stmt.name_.lexeme_;
    auto type = stmt.type_;
    auto resolved_type = registry_.resolve_type(type);

    if (resolved_type.is_unknown()) {
        if (type.is_struct()) reporter_.error(stmt, err::undefined_type, type.struct_name());
        return;
    }

    if (resolved_type.is_void()) {
        reporter_.error(stmt, err::void_variable);
        return;
    }

    if (symbols_.contains_in_current_scope(name)) {
        reporter_.error(stmt, err::redeclaration_variable, name);
        return;
    }

    if (stmt.initializer_) {
        auto init_type = type_of(*stmt.initializer_);
        if (init_type.is_unknown()) return;

        if (resolved_type.is_array() && resolved_type.array_size() == 0 && init_type.is_array()) {
            if (resolved_type.element_type().is_assignable_from(init_type.element_type())) {
                auto inferred_type = t::array_type(type.element_type(), init_type.array_size());
                symbols_.define(name, inferred_type);
                return;
            }
            reporter_.error(stmt, err::type_mismatch_initialization, name);
            return;
        }

        if (!resolved_type.is_assignable_from(init_type)) {
            reporter_.error(stmt, err::type_mismatch_initialization, name);
            return;
        }
    }

    symbols_.define(name, resolved_type);
}

void type_checker::check_block(const ast::block_stmt& stmt, bool create_scope) {
    std::optional<core::scope_guard<core::type>> guard;
    if (create_scope) guard.emplace(symbols_);
    for (const auto& s : stmt.statements_) check_statement(*s);
}

void type_checker::check_body(const ast::statement& body) {
    bool create_scope = false;
    if (auto* block = std::get_if<ast::block_stmt>(&body.data_)) {
        create_scope = has_declarations(*block);
        check_block(*block, create_scope);
    } else {
        check_statement(body);
    }
}

void type_checker::check_while(const ast::while_stmt& stmt) {
    auto cond_type = type_of(stmt.condition_);
    if (!cond_type.is_bool() && !cond_type.is_unknown()) reporter_.error(stmt, err::condition_not_bool, "while");

    core::scope_guard guard(symbols_);
    check_body(*stmt.block_);
}

void type_checker::check_for(const ast::for_stmt& stmt) {
    core::scope_guard guard(symbols_);

    if (stmt.initializer_) check_statement(*stmt.initializer_);
    if (stmt.condition_) {
        auto cond_type = type_of(*stmt.condition_);
        if (!cond_type.is_bool() && !cond_type.is_unknown()) reporter_.error(stmt, err::condition_not_bool, "for");
    }
    if (stmt.increment_) type_of(*stmt.increment_);
    check_body(*stmt.block_);
}

void type_checker::check_if(const ast::if_stmt& stmt) {
    auto cond_type = type_of(stmt.condition_);
    if (!cond_type.is_bool() && !cond_type.is_unknown()) reporter_.error(stmt, err::condition_not_bool, "if");
    check_body(*stmt.then_block_);
    if (stmt.else_block_) check_body(*stmt.else_block_);
}

void type_checker::check_return_stmt(const ast::return_stmt& stmt) {
    if (!curr_return_type_) {
        reporter_.error(stmt, err::return_outside_function);
        return;
    }

    if (!stmt.value_) {
        if (!curr_return_type_->is_void()) reporter_.error(stmt, err::return_missing_value);
        return;
    }

    auto return_type = type_of(*stmt.value_);
    if (return_type.is_unknown()) return;
    if (!curr_return_type_->is_assignable_from(return_type)) reporter_.error(stmt, err::return_type_mismatch);
}

void type_checker::check_func_declaration(const ast::func_declaration& stmt) {
    auto name = stmt.name_.lexeme_;

    if (symbols_.contains_in_current_scope(name)) {
        reporter_.error(stmt, err::redeclaration_function, name);
        return;
    }

    core::scope_guard guard(symbols_);
    for (const auto& param : stmt.params_) symbols_.define(param.name_.lexeme_, param.type_);

    const auto& prev_return_type = curr_return_type_;
    curr_return_type_ = stmt.return_type_;

    for (const auto& s : stmt.block_->statements_) check_statement(*s);

    curr_return_type_ = prev_return_type;
}

void type_checker::check_struct_declaration(const ast::struct_declaration& stmt) {
    auto name = stmt.name_.lexeme_;

    auto existing = registry_.find(name);
    if (existing) {
        bool is_duplicate = core::visit(core::overloaded{
                                            [&](core::symbol_registry::struct_ptr other) { return other != &stmt; },
                                            [](auto&&) { return true; },
                                        },
                                        existing->info_);

        if (is_duplicate) {
            reporter_.error(stmt, err::redeclaration_function, name);
            return;
        }
    }

    const auto& fields = stmt.type_.struct_fields();
    std::unordered_set<std::string_view> seen;
    for (const auto& [field_name, field_type] : fields) {
        if (!seen.insert(field_name).second) {
            reporter_.error(stmt, err::redeclaration_variable, field_name);
            return;
        }
        auto resolved = registry_.resolve_type(field_type);
        if (resolved.is_unknown() && field_type.is_struct()) {
            reporter_.error(stmt, err::undefined_type, field_type.struct_name());
        }
    }
}

// clang-format off
t type_checker::type_of(const ast::expression& expr) {
    return core::visit(
        core::overloaded{
            [this](const ast::literal_expr& e) { return type_of_literal(e); },
            [this](const ast::variable_expr& e) { return type_of_variable(e); },
            [this](const core::arena_ptr<ast::binary_expr>& e) { return type_of_binary(*e); },
            [this](const core::arena_ptr<ast::assignment_expr>& e) { return type_of_assignment(*e); },
            [this](const core::arena_ptr<ast::unary_expr>& e) { return type_of_unary(*e); },
            [this](const core::arena_ptr<ast::postfix_expr>& e) { return type_of_postfix(*e); },
            [this](const core::arena_ptr<ast::call_expr>& e) { return type_of_call(*e); },
            [this](const core::arena_ptr<ast::array_literal_expr>& e) { return type_of_array_literal(*e); },
            [this](const core::arena_ptr<ast::index_expr>& e) { return type_of_index(*e); },
            [this](const core::arena_ptr<ast::member_access_expr>& e) { return type_of_member_access(*e); }},
        expr);
}
// clang-format on
t type_checker::type_of_literal(const ast::literal_expr& expr) {
    const auto& token = expr.value_;

    switch (token.type_) {
        case tt::NUMBER:
            if (token.literal_value_ && token.literal_value_->is_double()) return t::double_type();
            return t::int_type();
        case tt::KW_TRUE:
        case tt::KW_FALSE:
            return t::bool_type();
        case tt::STRING:
            return t::string_type();
        default:
            reporter_.error(expr, err::unexpected_literal);
            return t::unknown_type();
    }
}

t type_checker::type_of_variable(const ast::variable_expr& expr_) {
    auto name = expr_.name_.lexeme_;
    auto info = symbols_.get(name);
    if (!info) {
        reporter_.error(expr_, err::undefined_variable, name);
        return t::unknown_type();
    }
    return *info;
}

t type_checker::type_of_binary(const ast::binary_expr& expr) {
    auto left = type_of(expr.left_);
    auto right = type_of(expr.right_);
    if (left.is_unknown() || right.is_unknown()) return t::unknown_type();

    auto op = expr.op_.type_;

    if (op == tt::PLUS || op == tt::MINUS || op == tt::STAR || op == tt::SLASH || op == tt::PERCENT) {
        if (!left.is_numeric() || !right.is_numeric()) {
            reporter_.error(expr, err::arithmetic_requires_numeric);
            return t::unknown_type();
        }
        return (left.is_int() && right.is_int()) ? t::int_type() : t::double_type();
    }

    if (op == tt::EQUAL_EQUAL || op == tt::BANG_EQUAL || op == tt::LESS || op == tt::LESS_EQUAL || op == tt::GREATER ||
        op == tt::GREATER_EQUAL) {
        if (!left.is_numeric() || !right.is_numeric()) {
            reporter_.error(expr, err::comparison_requires_numeric);
            return t::unknown_type();
        }
        return t::bool_type();
    }

    if (op == tt::BIT_AND || op == tt::BIT_OR || op == tt::XOR || op == tt::SHL || op == tt::SHR) {
        if (!left.is_int() || !right.is_int()) {
            reporter_.error(expr, err::arithmetic_requires_numeric);
            return t::unknown_type();
        }
        return t::int_type();
    }

    if (op == tt::LOGICAL_AND || op == tt::LOGICAL_OR) {
        if (!left.is_bool() || !right.is_bool()) {
            reporter_.error(expr, err::logical_requires_bool);
            return t::unknown_type();
        }
        return t::bool_type();
    }

    reporter_.error(expr, err::unsupported_binary_operator);
    return t::unknown_type();
}

t type_checker::type_of_assignment(const ast::assignment_expr& expr) {
    auto target_type = type_of(expr.target_);
    auto value_type = type_of(expr.value_);
    if (target_type.is_unknown() || value_type.is_unknown()) return t::unknown_type();

    if (expr.op_.type_ == tt::EQUAL) {
        if (!target_type.is_assignable_from(value_type) || !is_lvalue(expr.target_)) {
            reporter_.error(expr, err::type_mismatch_assignment);
            return t::unknown_type();
        }
        return target_type;
    }

    if (!is_lvalue(expr.target_)) {
        reporter_.error(expr, err::compound_requires_lvalue);
        return t::unknown_type();
    }
    if (!target_type.is_numeric() || !value_type.is_numeric()) {
        reporter_.error(expr, err::compound_requires_numeric);
        return t::unknown_type();
    }
    return target_type;
}

bool type_checker::is_lvalue(const ast::expression& expr) {
    return core::visit(
        core::overloaded{[](const ast::variable_expr&) { return true; },
                         [this](const core::arena_ptr<ast::index_expr>& idx) { return is_lvalue(idx->object_); },
                         [this](const core::arena_ptr<ast::member_access_expr>& e) { return is_lvalue(e->object_); },
                         [](const auto&) { return false; }},
        expr);
}

t type_checker::type_of_unary(const ast::unary_expr& expr) {
    auto operand_type = type_of(expr.operand_);
    if (operand_type.is_unknown()) return t::unknown_type();

    auto op = expr.op_.type_;

    if (op == tt::MINUS) {
        if (!operand_type.is_numeric()) {
            reporter_.error(expr, err::unary_minus_requires_numeric);
            return t::unknown_type();
        }
        return operand_type;
    }

    if (op == tt::INCREMENT || op == tt::DECREMENT) {
        if (!operand_type.is_numeric()) {
            reporter_.error(expr, err::increment_requires_numeric);
            return t::unknown_type();
        }
        if (!is_lvalue(expr.operand_)) {
            reporter_.error(expr, err::increment_requires_lvalue);
            return t::unknown_type();
        }
        return operand_type;
    }

    if (op == tt::BANG) {
        if (!operand_type.is_bool()) {
            reporter_.error(expr, err::not_requires_bool);
            return t::unknown_type();
        }
        return t::bool_type();
    }

    if (op == tt::BIT_NOT) {
        if (!operand_type.is_int()) {
            reporter_.error(expr, err::arithmetic_requires_numeric);
            return t::unknown_type();
        }
        return t::int_type();
    }

    reporter_.error(expr, err::unsupported_unary_operator);
    return t::unknown_type();
}

t type_checker::type_of_postfix(const ast::postfix_expr& expr) {
    auto operand_type = type_of(expr.operand_);
    if (operand_type.is_unknown()) return t::unknown_type();

    if (!operand_type.is_numeric()) {
        reporter_.error(expr, err::increment_requires_numeric);
        return t::unknown_type();
    }
    if (!is_lvalue(expr.operand_)) {
        reporter_.error(expr, err::increment_requires_lvalue);
        return t::unknown_type();
    }
    return operand_type;
}

t type_checker::type_of_call(const ast::call_expr& expr) {
    auto name = expr.callee_.lexeme_;

    auto func = registry_.find(name);
    if (!func) {
        reporter_.error(expr, err::undefined_function, name);
        return t::unknown_type();
    }

    const auto& param_types = func->type_.param_types();
    if (expr.args_.size() != param_types.size()) {
        reporter_.error(expr, err::argument_count_mismatch, name, param_types.size(), expr.args_.size());
        return t::unknown_type();
    }

    for (size_t i = 0; i < expr.args_.size(); ++i) {
        auto arg_type = type_of(expr.args_[i]);
        if (arg_type.is_unknown()) return t::unknown_type();
        if (!param_types[i].is_assignable_from(arg_type)) {
            reporter_.error(expr, err::argument_type_mismatch, i + 1, name);
            return t::unknown_type();
        }
    }

    return func->type_.return_type();
}

t type_checker::type_of_array_literal(const ast::array_literal_expr& expr) {
    if (expr.elements_.empty()) {
        reporter_.error(expr, err::empty_array_literal);
        return t::unknown_type();
    }

    auto elem_type = type_of(expr.elements_[0]);
    if (elem_type.is_unknown()) return t::unknown_type();

    for (size_t i = 1; i < expr.elements_.size(); i++) {
        auto el_type = type_of(expr.elements_[i]);
        if (el_type.is_unknown()) return t::unknown_type();
        if (!elem_type.is_assignable_from(el_type)) {
            reporter_.error(expr, err::array_literal_inconsistent_types);
            return t::unknown_type();
        }
    }
    return t::array_type(elem_type, expr.elements_.size());
}

t type_checker::type_of_index(const ast::index_expr& expr) {
    auto object_type = type_of(expr.object_);
    auto index_type = type_of(expr.index_);
    if (object_type.is_unknown() || index_type.is_unknown()) return t::unknown_type();

    if (!object_type.is_array()) {
        reporter_.error(expr, err::indexing_non_array);
        return t::unknown_type();
    }
    if (!index_type.is_int()) {
        reporter_.error(expr, err::index_must_be_integer);
        return t::unknown_type();
    }
    return object_type.element_type();
}

t type_checker::type_of_member_access(const ast::member_access_expr& expr) {
    auto obj_type = type_of(expr.object_);
    if (obj_type.is_unknown()) return t::unknown_type();

    obj_type = registry_.resolve_type(obj_type);

    if (obj_type.is_unknown()) return t::unknown_type();

    if (!obj_type.is_struct()) {
        reporter_.error(expr, err::not_a_struct);
        return t::unknown_type();
    }

    auto idx = obj_type.field_index(expr.member_.lexeme_);
    if (!idx) {
        reporter_.error(expr, err::no_such_field, obj_type.struct_name(), expr.member_.lexeme_);
        return t::unknown_type();
    }

    return obj_type.struct_fields()[*idx].second;
}

}  // namespace semantics
