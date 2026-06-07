// semantics/type_check.cpp


#include "semantics/type_check.hpp"
#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/utils/overloaded.hpp"
#include "core/token/token_types.hpp"
#include "core/token/keywords.hpp"
#include "core/utils/builtins.hpp"
#include "core/error/error_codes.hpp"
#include <string>
#include <algorithm>
#include <vector>
#include <ranges>

namespace semantics {

using tt = core::token_type;
using t = core::type;
using err = core::error_code;

type_checker::type_checker(core::error_reporter& reporter) : reporter_(reporter) {}

bool type_checker::check(const std::vector<std::unique_ptr<ast::statement>>& statements) {
    for (const auto& stmt : statements) check_statement(*stmt);
    return !reporter_.has_error();
}

void type_checker::check_statement(const ast::statement& stmt) {
    std::visit(core::overloaded{
        [this](const ast::expression_stmt& s) { check_expression_stmt(s); },
        [this](const ast::var_declaration& s) { check_var_declaration(s); },
        [this](const ast::block_stmt& s) { check_block(s); },
        [this](const ast::while_stmt& s) { check_while(s); },
        [this](const ast::for_stmt& s) { check_for(s); },
        [this](const ast::if_stmt& s) { check_if(s); },
        [this](const ast::return_stmt& s) { check_return_stmt(s); },
        [this](const ast::func_declaration& s) { check_func_declaration(s); },
        }, stmt.data_);
}

void type_checker::check_expression_stmt(const ast::expression_stmt& stmt) {
    type_of(stmt.expr_);
}

void type_checker::check_var_declaration(const ast::var_declaration& stmt) {
    std::string name{ stmt.name_.lexeme_ };

    if (stmt.type_.is_void()) {
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

        if (stmt.type_.is_array() && init_type.is_array()) {
            if (!stmt.type_.is_assignable_from(init_type)) {
                reporter_.error(stmt, err::type_mismatch_initialization, name);
                return;
            }
        } else if (!stmt.type_.is_assignable_from(init_type)) {
            reporter_.error(stmt, err::type_mismatch_initialization, name);
            return;
        }
    }

    symbols_.define(name, stmt.type_);
}

void type_checker::check_block(const ast::block_stmt& stmt) {
    symbols_.push();
    for (const auto& s : stmt.statements_) check_statement(*s);
    symbols_.pop();
}

void type_checker::check_while(const ast::while_stmt& stmt) {
    auto cond_type = type_of(stmt.condition_);
    if (cond_type != t::bool_type() && !cond_type.is_unknown()) {
        reporter_.error(stmt, err::condition_not_bool);
    }
    check_statement(*stmt.body_);
}

void type_checker::check_for(const ast::for_stmt& stmt) {
    symbols_.push();
    if (stmt.initializer_) check_statement(*stmt.initializer_);
    if (stmt.condition_) {
        auto cond_type = type_of(*stmt.condition_);
        if (cond_type != t::bool_type() && !cond_type.is_unknown()) {
            reporter_.error(stmt, err::condition_not_bool);
        }
    }
    if (stmt.increment_) type_of(*stmt.increment_);
    check_statement(*stmt.body_);
    symbols_.pop();
}

void type_checker::check_if(const ast::if_stmt& stmt) {
    auto cond_type = type_of(stmt.condition_);
    if (cond_type != t::bool_type() && !cond_type.is_unknown()) {
        reporter_.error(stmt, err::condition_not_bool);
    }
    check_statement(*stmt.then_branch_);
    if (stmt.else_branch_) check_statement(*stmt.else_branch_);
}

void type_checker::check_return_stmt(const ast::return_stmt& stmt) {
    if (!curr_return_type_) {
        reporter_.error(stmt, err::return_outside_function);
        return;
    }

    if (!stmt.value_) {
        if (!curr_return_type_->is_void()) {
            reporter_.error(stmt, err::return_missing_value);
        }
        return;
    }

    auto return_type = type_of(*stmt.value_);
    if (return_type.is_unknown()) return;
    if (!curr_return_type_->is_assignable_from(return_type)) {
        reporter_.error(stmt, err::return_type_mismatch);
    }
}

void type_checker::check_func_declaration(const ast::func_declaration& stmt) {
    std::string name{ stmt.name_.lexeme_ };

    if (symbols_.contains_in_current_scope(name)) {
        reporter_.error(stmt, err::redeclaration_function, name);
        return;
    }

    std::vector<t> param_types;
    param_types.reserve(stmt.params_.size());
    std::ranges::transform(stmt.params_, std::back_inserter(param_types),
        [](const auto& param) { return param.type_; });

    auto func_type = t::function_type(stmt.return_type_, param_types);
    symbols_.define_function(name, func_type);

    symbols_.push();

    for (const auto& param : stmt.params_) {
        std::string param_name{ param.name_.lexeme_ };
        symbols_.define(param_name, param.type_);
    }

    const auto& prev_return_type = curr_return_type_;
    curr_return_type_ = stmt.return_type_;

    for (const auto& s : stmt.body_->statements_) check_statement(*s);

    curr_return_type_ = prev_return_type;
    symbols_.pop();
}

t type_checker::type_of(const ast::expression& expr) {
    return std::visit(core::overloaded{
        [this](const ast::literal_expr& e) { return type_of_literal(e); },
        [this](const ast::variable_expr& e) { return type_of_variable(e); },
        [this](const std::unique_ptr<ast::binary_expr>& e) { return type_of_binary(*e); },
        [this](const std::unique_ptr<ast::unary_expr>& e) { return type_of_unary(*e); },
        [this](const std::unique_ptr<ast::postfix_expr>& e) { return type_of_postfix(*e); },
        [this](const std::unique_ptr<ast::call_expr>& e) { return type_of_call(*e); },
        [this](const std::unique_ptr<ast::array_literal_expr>& e) { return type_of_array_literal(*e); },
        [this](const std::unique_ptr<ast::index_expr>& e) { return type_of_index(*e); }
        }, expr);
}

t type_checker::type_of_literal(const ast::literal_expr& expr) {
    const auto& token = expr.value_;

    if (token.type_ == tt::NUMBER)
        return token.is_double_literal() ? t::double_type() : t::int_type();
    if (token.type_ == tt::STRING) return t::string_type();
    if (token.is_keyword()) {
        auto kw = token.as_keyword();
        if (kw && (kw->lexeme_ == "true" || kw->lexeme_ == "false")) return t::bool_type();
    }

    return reporter_.error_type(expr, err::unexpected_literal);
}

t type_checker::type_of_variable(const ast::variable_expr& expr_) {
    std::string name{ expr_.name_.lexeme_ };
    auto info = symbols_.get(name);
    if (!info) return reporter_.error_type(expr_, err::undefined_variable, name);
    return info->type_;
}

t type_checker::type_of_binary(const ast::binary_expr& expr) {
    auto left = type_of(expr.left_);
    auto right = type_of(expr.right_);
    if (left.is_unknown() || right.is_unknown()) return t::unknown_type();

    auto op = expr.op_.type_;

    if (op == tt::EQUAL) {
        if (!left.is_assignable_from(right) || !is_lvalue(expr.left_))
            return reporter_.error_type(expr, err::type_mismatch_assignment);
        return left;
    }

    if (op == tt::PLUS_EQUAL || op == tt::MINUS_EQUAL ||
        op == tt::STAR_EQUAL || op == tt::SLASH_EQUAL ||
        op == tt::PERCENT_EQUAL) {
        if (!is_lvalue(expr.left_))
            return reporter_.error_type(expr, err::compound_requires_lvalue);
        if (!left.is_numeric() || !right.is_numeric())
            return reporter_.error_type(expr, err::compound_requires_numeric);
        return left;
    }

    if (op == tt::PLUS || op == tt::MINUS ||
        op == tt::STAR || op == tt::SLASH ||
        op == tt::PERCENT) {
        if (!left.is_numeric() || !right.is_numeric())
            return reporter_.error_type(expr, err::arithmetic_requires_numeric);
        return (left == t::int_type() && right == t::int_type())
            ? t::int_type() : t::double_type();
    }

    if (op == tt::EQUAL_EQUAL || op == tt::BANG_EQUAL ||
        op == tt::LESS || op == tt::LESS_EQUAL ||
        op == tt::GREATER || op == tt::GREATER_EQUAL) {
        if (!left.is_numeric() || !right.is_numeric())
            return reporter_.error_type(expr, err::comparison_requires_numeric);
        return t::bool_type();
    }

    if (op == tt::AND || op == tt::OR) {
        if (left != t::bool_type() || right != t::bool_type())
            return reporter_.error_type(expr, err::logical_requires_bool);
        return t::bool_type();
    }

    return reporter_.error_type(expr, err::unsupported_binary_operator);
}

bool type_checker::is_lvalue(const ast::expression& expr) {
    if (std::holds_alternative<ast::variable_expr>(expr)) return true;
    if (auto* idx = std::get_if<std::unique_ptr<ast::index_expr>>(&expr)) {
        return is_lvalue((*idx)->object_);
    }
    return false;
}

t type_checker::type_of_unary(const ast::unary_expr& expr) {
    auto operand_type = type_of(expr.operand_);
    if (operand_type.is_unknown()) return t::unknown_type();

    auto op = expr.op_.type_;

    if (op == tt::MINUS) {
        if (!operand_type.is_numeric())
            return reporter_.error_type(expr, err::unary_minus_requires_numeric);
        return operand_type;
    }

    if (op == tt::INCREMENT || op == tt::DECREMENT) {
        if (!operand_type.is_numeric())
            return reporter_.error_type(expr, err::increment_requires_numeric);
        if (!is_lvalue(expr.operand_))
            return reporter_.error_type(expr, err::increment_requires_lvalue);
        return operand_type;
    }

    if (op == tt::BANG) {
        if (operand_type != t::bool_type())
            return reporter_.error_type(expr, err::not_requires_bool);
        return t::bool_type();
    }

    return reporter_.error_type(expr, err::unsupported_unary_operator);
}

t type_checker::type_of_postfix(const ast::postfix_expr& expr) {
    auto operand_type = type_of(expr.operand_);
    if (operand_type.is_unknown()) return t::unknown_type();

    if (!operand_type.is_numeric())
        return reporter_.error_type(expr, err::increment_requires_numeric);
    if (!is_lvalue(expr.operand_))
        return reporter_.error_type(expr, err::increment_requires_lvalue);
    return operand_type;
}

t type_checker::type_of_call(const ast::call_expr& expr) {
    std::string name{ expr.callee_.lexeme_ };

    std::vector<t> arg_types;
    arg_types.reserve(expr.args_.size());
    std::ranges::transform(expr.args_, std::back_inserter(arg_types),
        [this](const auto& arg) { return type_of(arg); });

    for (const auto& def : core::builtins) {
        if (def.name_ != name) continue;
        for (const auto& ov : def.overloads_) {
            if (ov.param_types_.size() == arg_types.size() &&
                std::ranges::equal(ov.param_types_, arg_types,
                    [](const auto& p, const auto& a) { return p.is_assignable_from(a); })) {
                return ov.return_type_;
            }
        }
        return reporter_.error_type(expr, err::no_matching_overload, name);
    }

    auto info = symbols_.get(name);
    if (!info || info->kind_ != symbol_kind::FUNCTION)
        return reporter_.error_type(expr, err::undefined_function, name);

    const auto& func_type = info->type_;
    const auto& param_types = func_type.param_types();

    if (expr.args_.size() != param_types.size()) {
        return reporter_.error_type(expr, err::argument_count_mismatch, name,
            param_types.size(), expr.args_.size());
    }

    for (size_t i = 0; i < expr.args_.size(); i++) {
        auto arg_type = type_of(expr.args_[i]);
        if (arg_type.is_unknown()) return t::unknown_type();
        if (!param_types[i].is_assignable_from(arg_type)) {
            return reporter_.error_type(expr, err::argument_type_mismatch, i + 1, name);
        }
    }

    return func_type.return_type();
}

t type_checker::type_of_array_literal(const ast::array_literal_expr& expr) {
    if (expr.elements_.empty())
        return reporter_.error_type(expr, err::empty_array_literal);

    auto elem_type = type_of(expr.elements_[0]);
    if (elem_type.is_unknown()) return t::unknown_type();

    for (size_t i = 1; i < expr.elements_.size(); i++) {
        auto el_type = type_of(expr.elements_[i]);
        if (el_type.is_unknown()) return t::unknown_type();
        if (!elem_type.is_assignable_from(el_type))
            return reporter_.error_type(expr, err::array_literal_inconsistent_types);
    }
    return t::array_type(elem_type, expr.elements_.size());
}

t type_checker::type_of_index(const ast::index_expr& expr) {
    auto object_type = type_of(expr.object_);
    auto index_type = type_of(expr.index_);
    if (object_type.is_unknown() || index_type.is_unknown()) return t::unknown_type();

    if (!object_type.is_array())
        return reporter_.error_type(expr, err::indexing_non_array);
    if (!index_type.is_int())
        return reporter_.error_type(expr, err::index_must_be_integer);
    return object_type.element_type();
}

} // namespace semantics