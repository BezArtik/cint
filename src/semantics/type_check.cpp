// semantics/type_check.cpp

#include "semantics/type_check.hpp"

#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/error/error_codes.hpp"
#include "core/symbol/symbol_registry.hpp"
#include "core/token/token_types.hpp"
#include "core/utils/overloaded.hpp"
#include "core/utils/scoped_map.hpp"

#include <unordered_set>

using tt = core::token_type;
using t = core::type;
using err = core::error_code;

bool type_checker::check(std::span<const ast::statement> statements) {
    for (auto&& stmt : statements) check_statement(stmt);
    return !reporter_.has_error();
}
// clang-format off
void type_checker::check_statement(const ast::statement& stmt) {
    stmt.visit(core::overloaded{
            [&](const ast::expression_stmt& s) { check_expression_stmt(s); },
            [&](const ast::var_declaration_stmt& s) { check_var_declaration(s); },
            [&](const ast::block_stmt& s) { check_block(s); },
            [&](const ast::while_stmt& s) { check_while(s); },
            [&](const ast::for_stmt& s) { check_for(s); },
            [&](const ast::if_stmt& s) { check_if(s); },
            [&](const ast::return_stmt& s) { check_return_stmt(s); },
            [&](const ast::func_declaration_stmt& s) { check_func_declaration(s); },
            [&](const ast::struct_declaration_stmt& s) { check_struct_declaration(s); }
            });
}
// clang-format on
void type_checker::check_expression_stmt(const ast::expression_stmt& stmt) {
    type_of(stmt.expr_);
}

void type_checker::check_var_declaration(const ast::var_declaration_stmt& stmt) {
    auto&& name = stmt.name_.lexeme_;
    auto&& type = stmt.type_;
    auto&& loc = stmt.name_.loc_;
    auto&& resolved_type = registry_.resolve_type(type);

    if (resolved_type.is_unknown()) {
        if (type.is_struct()) reporter_.error(loc, err::undefined_type, type.struct_name());
        return;
    }

    if (resolved_type.is_void()) {
        reporter_.error(loc, err::void_variable);
        return;
    }

    if (symbols_.contains_in_current_scope(name)) {
        reporter_.error(loc, err::redeclaration, name);
        return;
    }

    if (stmt.initializer_) {
        auto&& init_type = type_of(*stmt.initializer_);
        if (init_type.is_unknown()) return;

        if (resolved_type.is_struct()) {
            if (auto&& list = stmt.initializer_->get_if<ast::initializer_list_expr>()) {
                auto&& fields = resolved_type.struct_fields();
                if (list->elements_.size() > fields.size()) {
                    reporter_.error(loc, err::argument_count_mismatch, "initializer", fields.size(),
                                    list->elements_.size());
                    return;
                }
                for (size_t i = 0; i < list->elements_.size(); ++i) {
                    auto&& elem_type = type_of(list->elements_[i]);
                    if (elem_type.is_unknown()) continue;
                    if (fields[i].second != elem_type) {
                        reporter_.error(loc, err::type_mismatch_initialization, name);
                        return;
                    }
                }
                symbols_.define(name, resolved_type);
                return;
            }
        }

        if (resolved_type.is_array() && resolved_type.array_size() == 0 && init_type.is_array()) {
            if (resolved_type.element_type() == init_type.element_type()) {
                auto&& inferred_type = t::array_type(type.element_type(), init_type.array_size());
                symbols_.define(name, inferred_type);
                return;
            }
            reporter_.error(loc, err::type_mismatch_initialization, name);
            return;
        }

        if (resolved_type != init_type) {
            reporter_.error(loc, err::type_mismatch_initialization, name);
            return;
        }
    }

    symbols_.define(name, resolved_type);
}

void type_checker::check_block(const ast::block_stmt& stmt, bool create_scope) {
    std::optional<core::scope_guard<core::type>> guard;
    if (create_scope) guard.emplace(symbols_);
    for (auto&& s : stmt.statements_) check_statement(s);
}

void type_checker::check_body(const ast::statement& body) {
    if (auto&& block = body.get_if<ast::block_stmt>())
        check_block(*block, block->has_declarations_);
    else
        check_statement(body);
}

void type_checker::check_while(const ast::while_stmt& stmt) {
    auto&& cond_type = type_of(stmt.condition_);
    if (!cond_type.is_bool() && !cond_type.is_unknown()) reporter_.error(stmt.loc_, err::condition_not_bool, "while");

    core::scope_guard guard{symbols_};
    check_body(stmt.block_);
}

void type_checker::check_for(const ast::for_stmt& stmt) {
    core::scope_guard guard{symbols_};

    if (stmt.initializer_) check_statement(*stmt.initializer_);
    if (stmt.condition_) {
        auto&& cond_type = type_of(*stmt.condition_);
        if (!cond_type.is_bool() && !cond_type.is_unknown()) reporter_.error(stmt.loc_, err::condition_not_bool, "for");
    }
    if (stmt.increment_) type_of(*stmt.increment_);
    check_body(stmt.block_);
}

void type_checker::check_if(const ast::if_stmt& stmt) {
    auto&& cond_type = type_of(stmt.condition_);
    if (!cond_type.is_bool() && !cond_type.is_unknown()) reporter_.error(stmt.loc_, err::condition_not_bool, "if");
    check_body(stmt.then_block_);
    if (stmt.else_block_) check_body(*stmt.else_block_);
}

void type_checker::check_return_stmt(const ast::return_stmt& stmt) {
    if (!curr_return_type_) {
        reporter_.error(stmt.loc_, err::return_outside_function);
        return;
    }

    if (!stmt.value_) {
        if (!curr_return_type_->is_void()) reporter_.error(stmt.loc_, err::return_missing_value);
        return;
    }

    auto&& return_type = type_of(*stmt.value_);
    if (return_type.is_unknown()) return;
    if (*curr_return_type_ != return_type) reporter_.error(stmt.loc_, err::return_type_mismatch);
}

void type_checker::check_func_declaration(const ast::func_declaration_stmt& stmt) {
    core::scope_guard guard{symbols_};
    for (auto&& [name, type] : stmt.type_.param_infos()) symbols_.define(name, type);

    auto&& prev_return_type = curr_return_type_;
    curr_return_type_ = stmt.type_.return_type();

    auto&& block = stmt.block_.get<ast::block_stmt>();
    for (auto&& s : block.statements_) check_statement(s);

    curr_return_type_ = prev_return_type;
}

void type_checker::check_struct_declaration(const ast::struct_declaration_stmt& stmt) {
    std::unordered_set<std::string_view> seen;
    for (auto&& [name, type] : stmt.type_.struct_fields()) {
        if (!seen.insert(name).second) {
            reporter_.error(stmt.loc_, err::redeclaration, name);
            return;
        }
        auto&& resolved = registry_.resolve_type(type);
        if (resolved.is_unknown() && type.is_struct())
            reporter_.error(stmt.loc_, err::undefined_type, type.struct_name());
    }
}

// clang-format off
t type_checker::type_of(const ast::expression& expr) {
    return expr.visit(
        core::overloaded{
            [&](const ast::literal_expr& e) { return type_of_literal(e); },
            [&](const ast::variable_expr& e) { return type_of_variable(e); },
            [&](const ast::binary_expr& e) { return type_of_binary(e); },
            [&](const ast::assignment_expr& e) { return type_of_assignment(e); },
            [&](const ast::unary_expr& e) { return type_of_unary(e); },
            [&](const ast::postfix_expr& e) { return type_of_postfix(e); },
            [&](const ast::call_expr& e) { return type_of_call(e); },
            [&](const ast::initializer_list_expr& e) { return type_of_initializer_list(e); },
            [&](const ast::index_expr& e) { return type_of_index(e); },
            [&](const ast::member_access_expr& e) { return type_of_member_access(e); }
            });
}
// clang-format on
t type_checker::type_of_literal(const ast::literal_expr& expr) {
    auto&& val = expr.value_;

    if (val.is_int()) return t::int_type();
    if (val.is_double()) return t::double_type();
    if (val.is_bool()) return t::bool_type();
    if (val.is_string()) return t::string_type();

    reporter_.error(expr.loc_, err::unexpected_literal);
    return t::unknown_type();
}

t type_checker::type_of_variable(const ast::variable_expr& expr_) {
    auto&& name = expr_.name_.lexeme_;
    auto&& info = symbols_.get(name);
    if (!info) {
        reporter_.error(expr_.name_.loc_, err::undefined_variable, name);
        return t::unknown_type();
    }
    return *info;
}

t type_checker::type_of_binary(const ast::binary_expr& expr) {
    auto&& left = type_of(expr.left_);
    auto&& right = type_of(expr.right_);
    if (left.is_unknown() || right.is_unknown()) return t::unknown_type();

    auto&& op = expr.op_.type_;
    auto&& loc = expr.op_.loc_;

    if (op == tt::PLUS || op == tt::MINUS || op == tt::STAR || op == tt::SLASH || op == tt::PERCENT) {
        if (!left.is_numeric() || !right.is_numeric()) {
            reporter_.error(loc, err::arithmetic_requires_numeric);
            return t::unknown_type();
        }
        return (left.is_int() && right.is_int()) ? t::int_type() : t::double_type();
    }

    if (op == tt::EQUAL_EQUAL || op == tt::BANG_EQUAL || op == tt::LESS || op == tt::LESS_EQUAL || op == tt::GREATER ||
        op == tt::GREATER_EQUAL) {
        if (!left.is_numeric() || !right.is_numeric()) {
            reporter_.error(loc, err::comparison_requires_numeric);
            return t::unknown_type();
        }
        return t::bool_type();
    }

    if (op == tt::BIT_AND || op == tt::BIT_OR || op == tt::XOR || op == tt::SHL || op == tt::SHR) {
        if (!left.is_int() || !right.is_int()) {
            reporter_.error(loc, err::arithmetic_requires_numeric);
            return t::unknown_type();
        }
        return t::int_type();
    }

    if (op == tt::LOGICAL_AND || op == tt::LOGICAL_OR) {
        if (!left.is_bool() || !right.is_bool()) {
            reporter_.error(loc, err::logical_requires_bool);
            return t::unknown_type();
        }
        return t::bool_type();
    }

    reporter_.error(loc, err::unsupported_binary_operator);
    return t::unknown_type();
}

t type_checker::type_of_assignment(const ast::assignment_expr& expr) {
    auto&& target_type = type_of(expr.target_);
    auto&& value_type = type_of(expr.value_);
    if (target_type.is_unknown() || value_type.is_unknown()) return t::unknown_type();

    auto&& loc = expr.op_.loc_;

    if (expr.op_.type_ == tt::EQUAL) {
        if (target_type != value_type || !is_lvalue(expr.target_)) {
            reporter_.error(loc, err::type_mismatch_assignment);
            return t::unknown_type();
        }
        return target_type;
    }

    if (!is_lvalue(expr.target_)) {
        reporter_.error(loc, err::compound_requires_lvalue);
        return t::unknown_type();
    }
    if (!target_type.is_numeric() || !value_type.is_numeric()) {
        reporter_.error(loc, err::compound_requires_numeric);
        return t::unknown_type();
    }
    return target_type;
}

bool type_checker::is_lvalue(const ast::expression& expr) {
    return expr.visit(core::overloaded{[](const ast::variable_expr&) { return true; },
                                       [&](const ast::index_expr& idx) { return is_lvalue(idx.object_); },
                                       [&](const ast::member_access_expr& e) { return is_lvalue(e.object_); },
                                       [](const auto&) { return false; }});
}

t type_checker::type_of_unary(const ast::unary_expr& expr) {
    auto&& operand_type = type_of(expr.operand_);
    if (operand_type.is_unknown()) return t::unknown_type();

    auto&& op = expr.op_.type_;
    auto&& loc = expr.op_.loc_;

    if (op == tt::MINUS) {
        if (!operand_type.is_numeric()) {
            reporter_.error(loc, err::unary_minus_requires_numeric);
            return t::unknown_type();
        }
        return operand_type;
    }

    if (op == tt::INCREMENT || op == tt::DECREMENT) {
        if (!operand_type.is_numeric()) {
            reporter_.error(loc, err::increment_requires_numeric);
            return t::unknown_type();
        }
        if (!is_lvalue(expr.operand_)) {
            reporter_.error(loc, err::increment_requires_lvalue);
            return t::unknown_type();
        }
        return operand_type;
    }

    if (op == tt::BANG) {
        if (!operand_type.is_bool()) {
            reporter_.error(loc, err::not_requires_bool);
            return t::unknown_type();
        }
        return t::bool_type();
    }

    if (op == tt::BIT_NOT) {
        if (!operand_type.is_int()) {
            reporter_.error(loc, err::arithmetic_requires_numeric);
            return t::unknown_type();
        }
        return t::int_type();
    }

    reporter_.error(loc, err::unsupported_unary_operator);
    return t::unknown_type();
}

t type_checker::type_of_postfix(const ast::postfix_expr& expr) {
    auto&& operand_type = type_of(expr.operand_);
    if (operand_type.is_unknown()) return t::unknown_type();

    auto&& loc = expr.op_.loc_;

    if (!operand_type.is_numeric()) {
        reporter_.error(loc, err::increment_requires_numeric);
        return t::unknown_type();
    }
    if (!is_lvalue(expr.operand_)) {
        reporter_.error(loc, err::increment_requires_lvalue);
        return t::unknown_type();
    }
    return operand_type;
}

t type_checker::type_of_call(const ast::call_expr& expr) {
    auto&& name = expr.callee_.lexeme_;
    auto&& loc = expr.callee_.loc_;

    auto&& func = registry_.find(name);

    if (func == registry_.end()) {
        reporter_.error(loc, err::undefined_function, name);
        return t::unknown_type();
    }

    auto&& params = func->type_.param_infos();
    if (expr.args_.size() != params.size()) {
        reporter_.error(loc, err::argument_count_mismatch, name, params.size(), expr.args_.size());
        return t::unknown_type();
    }

    for (size_t i = 0; i < expr.args_.size(); ++i) {
        auto&& arg_type = type_of(expr.args_[i]);
        if (arg_type.is_unknown()) return t::unknown_type();
        if (params[i].second != arg_type) {
            reporter_.error(loc, err::argument_type_mismatch, i + 1, name);
            return t::unknown_type();
        }
    }

    return func->type_.return_type();
}

t type_checker::type_of_initializer_list(const ast::initializer_list_expr& expr) {
    if (expr.elements_.empty()) {
        reporter_.error(expr.loc_, err::empty_initializer_list);
        return t::unknown_type();
    }

    auto&& first_type = type_of(expr.elements_[0]);
    if (first_type.is_unknown()) return t::unknown_type();

    for (size_t i = 1; i < expr.elements_.size(); ++i) {
        auto&& el_type = type_of(expr.elements_[i]);
        if (el_type.is_unknown()) return t::unknown_type();
        if (first_type != el_type) {
            reporter_.error(expr.loc_, err::initializer_list_inconsistent_types);
            return t::unknown_type();
        }
    }

    return t::array_type(first_type, expr.elements_.size());
}

t type_checker::type_of_index(const ast::index_expr& expr) {
    auto&& object_type = type_of(expr.object_);
    auto&& index_type = type_of(expr.index_);
    if (object_type.is_unknown() || index_type.is_unknown()) return t::unknown_type();

    if (!object_type.is_array()) {
        reporter_.error(expr.loc_, err::indexing_non_array);
        return t::unknown_type();
    }
    if (!index_type.is_int()) {
        reporter_.error(expr.loc_, err::index_must_be_integer);
        return t::unknown_type();
    }
    return object_type.element_type();
}

t type_checker::type_of_member_access(const ast::member_access_expr& expr) {
    auto&& obj_type = type_of(expr.object_);
    if (obj_type.is_unknown()) return t::unknown_type();

    obj_type = registry_.resolve_type(obj_type);

    if (obj_type.is_unknown()) return t::unknown_type();

    auto&& loc = expr.member_.loc_;
    auto&& lex = expr.member_.lexeme_;

    if (!obj_type.is_struct()) {
        reporter_.error(loc, err::not_a_struct);
        return t::unknown_type();
    }

    auto&& idx = obj_type.field_index(lex);
    if (!idx) {
        reporter_.error(loc, err::no_such_field, obj_type.struct_name(), lex);
        return t::unknown_type();
    }

    return obj_type.struct_fields()[*idx].second;
}
