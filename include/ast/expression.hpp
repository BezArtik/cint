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
    uint32_t line_{};
    uint32_t column_{};

    literal_expr(const core::token& value, uint32_t line, uint32_t column)
        : value_(value), line_(line), column_(column) {
    }
};

struct variable_expr {
    core::token name_{};
    uint32_t line_{};
    uint32_t column_{};

    variable_expr(const core::token& name, uint32_t line, uint32_t column)
        : name_(name), line_(line), column_(column) {
    }
};

struct binary_expr {
    expression left_; 
    core::token op_{};
    expression right_;
    uint32_t line_{};
    uint32_t column_{};

    binary_expr(expression left, const core::token& op, expression right, uint32_t line, uint32_t column)
        : left_(std::move(left)), op_(op), right_(std::move(right)), line_(line), column_(column) {
    }
};

struct unary_expr {
    core::token op_{};
    expression operand_;
    uint32_t line_{};
    uint32_t column_{};

    unary_expr(const core::token& op, expression operand, uint32_t line, uint32_t column)
        : op_(op), operand_(std::move(operand)), line_(line), column_(column) {
    }
};

struct postfix_expr {
    expression operand_;
    core::token op_{};
    uint32_t line_{};
    uint32_t column_{};

    postfix_expr(expression operand, core::token op, uint32_t line, uint32_t column)
        : operand_(std::move(operand)), op_(op), line_(line), column_(column) {
    }
};

struct call_expr {
    core::token callee_{};
    std::vector<expression> args_{};
    uint32_t line_{};
    uint32_t column_{};

    call_expr(const core::token& callee, std::vector<expression> args,
        uint32_t line, uint32_t column)
        : callee_(callee), args_(std::move(args)), line_(line), column_(column) {
    }
};

struct array_literal_expr {
	std::vector<expression> elements_{};
	uint32_t line_{};
	uint32_t column_{};

	array_literal_expr(std::vector<expression> elements, uint32_t line, uint32_t column)
		: elements_(std::move(elements)), line_(line), column_(column) {
	}
};

struct index_expr {
    expression object_;
    expression index_;
	uint32_t line_{};
	uint32_t column_{};

	index_expr(expression object, expression index, uint32_t line, uint32_t column)
		: object_(std::move(object)), index_(std::move(index)), line_(line), column_(column) {
	}
};

} // namespace ast