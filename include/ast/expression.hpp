// ast/expression.hpp

#pragma once

#include "core/token/token.hpp"
#include <variant>
#include <memory>
#include <vector>
#include <cstdint>

namespace ast {

struct literal_expr;
struct variable_expr;
struct binary_expr;
struct unary_expr;
struct postfix_expr;
struct call_expr;
struct array_literal_expr;
struct index_expr;

using expression = std::variant<
    literal_expr,
    variable_expr,
    std::unique_ptr<binary_expr>,
    std::unique_ptr<unary_expr>,
    std::unique_ptr<postfix_expr>,
    std::unique_ptr<call_expr>,
	std::unique_ptr<array_literal_expr>,
	std::unique_ptr<index_expr>
>;

struct literal_expr {
    core::token value_{};
    core::location loc_{};

    literal_expr(const core::token& value, core::location loc)
        : value_(value), loc_(loc) {}
};

struct variable_expr {
    core::token name_{};
    core::location loc_{};

    variable_expr(const core::token& name, core::location loc)
        : name_(name), loc_(loc) {}
};

struct binary_expr {
    expression left_; 
    core::token op_{};
    expression right_;
    core::location loc_{};

    binary_expr(expression left, const core::token& op, expression right, core::location loc)
        : left_(std::move(left)), op_(op), right_(std::move(right)), loc_(loc) {}
};

struct unary_expr {
    core::token op_{};
    expression operand_;
    core::location loc_{};

    unary_expr(const core::token& op, expression operand, core::location loc)
        : op_(op), operand_(std::move(operand)), loc_(loc) {}
};

struct postfix_expr {
    expression operand_;
    core::token op_{};
    core::location loc_{};

    postfix_expr(expression operand, core::token op, core::location loc)
        : operand_(std::move(operand)), op_(op), loc_(loc) {}
};

struct call_expr {
    core::token callee_{};
    std::vector<expression> args_{};
    core::location loc_{};

    call_expr(const core::token& callee, std::vector<expression> args, core::location loc)
        : callee_(callee), args_(std::move(args)), loc_(loc) {}
};

struct array_literal_expr {
	std::vector<expression> elements_{};
    core::location loc_{};

	array_literal_expr(std::vector<expression> elements, core::location loc)
		: elements_(std::move(elements)), loc_(loc) {}
};

struct index_expr {
    expression object_;
    expression index_;
    core::location loc_{};

	index_expr(expression object, expression index, core::location loc)
        : object_(std::move(object)), index_(std::move(index)), loc_(loc) {}
};

} // namespace ast