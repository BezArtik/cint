// ast/expression.hpp

#pragma once

#include "core/token/token.hpp"
#include "core/utils/arena.hpp"

#include <variant>
#include <vector>

namespace ast {

struct literal_expr;
struct variable_expr;
struct binary_expr;
struct assignment_expr;
struct unary_expr;
struct postfix_expr;
struct call_expr;
struct array_literal_expr;
struct index_expr;

using expression =
    std::variant<literal_expr, variable_expr, core::arena_ptr<binary_expr>, core::arena_ptr<assignment_expr>,
                 core::arena_ptr<unary_expr>, core::arena_ptr<postfix_expr>, core::arena_ptr<call_expr>,
                 core::arena_ptr<array_literal_expr>, core::arena_ptr<index_expr> >;

using expr_list = std::pmr::vector<expression>;

struct literal_expr {
    core::token value_;
    core::location loc_;

    literal_expr(const core::token& value, core::location loc) : value_(value), loc_(loc) {}
};

struct variable_expr {
    core::token name_;
    core::location loc_;

    variable_expr(const core::token& name, core::location loc) : name_(name), loc_(loc) {}
};

struct binary_expr {
    expression left_;
    core::token op_;
    expression right_;
    core::location loc_;

    binary_expr(expression left, const core::token& op, expression right, core::location loc)
        : left_(std::move(left)), op_(op), right_(std::move(right)), loc_(loc) {}
};

struct assignment_expr {
    expression target_;
    core::token op_;
    expression value_;
    core::location loc_;

    assignment_expr(expression target, const core::token& op, expression value, core::location loc)
        : target_(std::move(target)), op_(op), value_(std::move(value)), loc_(loc) {}
};

struct unary_expr {
    core::token op_;
    expression operand_;
    core::location loc_;

    unary_expr(const core::token& op, expression operand, core::location loc)
        : op_(op), operand_(std::move(operand)), loc_(loc) {}
};

struct postfix_expr {
    expression operand_;
    core::token op_;
    core::location loc_;

    postfix_expr(expression operand, core::token op, core::location loc)
        : operand_(std::move(operand)), op_(op), loc_(loc) {}
};

struct call_expr {
    core::token callee_;
    expr_list args_;
    core::location loc_;

    call_expr(const core::token& callee, expr_list args, core::location loc)
        : callee_(callee), args_(std::move(args)), loc_(loc) {}
};

struct array_literal_expr {
    expr_list elements_;
    core::location loc_;

    array_literal_expr(expr_list elements, core::location loc) : elements_(std::move(elements)), loc_(loc) {}
};

struct index_expr {
    expression object_;
    expression index_;
    core::location loc_;

    index_expr(expression object, expression index, core::location loc)
        : object_(std::move(object)), index_(std::move(index)), loc_(loc) {}
};

template <typename T, typename... Args>
expression make_expr(core::arena& arena, core::location loc, Args&&... args) {
    auto* p = arena.allocate<T>(std::forward<Args>(args)..., loc);
    return expression(core::arena_ptr<T>(p));
}

template <typename T, typename... Args>
expression make_expr_val(const core::token& tok, Args&&... args) {
    T val(std::forward<Args>(args)..., tok, tok.loc_);
    return expression(std::move(val));
}

}  // namespace ast
