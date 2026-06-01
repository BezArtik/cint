// ast/expression.hpp

#pragma once

#include "core/token/token.hpp"
#include <variant>
#include <memory>
#include <vector>

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
    size_t line_{};
    size_t column_{};

    literal_expr() = default;
    literal_expr(const core::token& value, size_t line, size_t column)
        : value_(value), line_(line), column_(column) {
    }
};

struct variable_expr {
    core::token name_{};
    size_t line_{};
    size_t column_{};

    variable_expr() = default;
    variable_expr(const core::token& name, size_t line, size_t column)
        : name_(name), line_(line), column_(column) {
    }
};

struct binary_expr {
    expression left_; 
    core::token op_{};
    expression right_;
    size_t line_{};
    size_t column_{};

    binary_expr() = default;
    binary_expr(expression left, const core::token& op, expression right, size_t line, size_t column)
        : left_(std::move(left)), op_(op), right_(std::move(right)), line_(line), column_(column) {
    }
};

struct unary_expr {
    core::token op_{};
    expression operand_;
    size_t line_{};
    size_t column_{};

    unary_expr() = default;
    unary_expr(const core::token& op, expression operand, size_t line, size_t column)
        : op_(op), operand_(std::move(operand)), line_(line), column_(column) {
    }
};

struct postfix_expr {
    expression operand_;
    core::token op_{};
    size_t line_{};
    size_t column_{};

    postfix_expr() = default;
    postfix_expr(expression operand, core::token op, size_t line, size_t column)
        : operand_(std::move(operand)), op_(op), line_(line), column_(column) {
    }
};

struct call_expr {
    core::token callee_{};
    std::vector<expression> args_{};
    size_t line_{};
    size_t column_{};

    call_expr() = default;
    call_expr(const core::token& callee, std::vector<expression> args,
        size_t line, size_t column)
        : callee_(callee), args_(std::move(args)), line_(line), column_(column) {
    }
};

struct array_literal_expr {
	std::vector<expression> elements_{};
	size_t line_{};
	size_t column_{};

	array_literal_expr() = default;
	array_literal_expr(std::vector<expression> elements, size_t line, size_t column)
		: elements_(std::move(elements)), line_(line), column_(column) {
	}
};

struct index_expr {
    expression object_;
    expression index_;
	size_t line_{};
	size_t column_{};

	index_expr() = default;
	index_expr(expression object, expression index, size_t line, size_t column)
		: object_(std::move(object)), index_(std::move(index)), line_(line), column_(column) {
	}
};

} // namespace ast