// ast/statement.hpp

#pragma once

#include "ast/expression.hpp"
#include "core/token/token_types.hpp"
#include <vector>
#include <variant>
#include <memory>

namespace ast {

struct statement;

using stmt_ptr = std::unique_ptr<statement>;

struct expression_stmt {
    expression expr_;
    uint32_t line_{};
    uint32_t column_{};

    expression_stmt(expression e, uint32_t line, uint32_t column)
        : expr_(std::move(e)), line_(line), column_(column) {
    }
};

struct var_declaration {
    core::type type_{};
    core::token name_{};
    std::optional<expression> initializer_{};
	uint32_t line_{};
	uint32_t column_{};

    var_declaration(core::type t, const core::token& n, 
                    std::optional<expression> init = std::nullopt, 
                    uint32_t line = 0, uint32_t column = 0)
        : type_(std::move(t)), name_(n), initializer_(std::move(init)), line_(line), column_(column) {
    }
};

struct block_stmt {
    std::vector<stmt_ptr> statements_{};
    uint32_t line_{};
    uint32_t column_{};

    block_stmt() = default;
    block_stmt(uint32_t line, uint32_t column) : line_(line), column_(column) {}
};

struct while_stmt {
    expression condition_;
    stmt_ptr body_{};
    uint32_t line_{};
    uint32_t column_{};

    while_stmt(expression cond, stmt_ptr body, uint32_t line, uint32_t column)
        : condition_(std::move(cond)), body_(std::move(body)), line_(line), column_(column) {
    }
};

struct for_stmt {
    stmt_ptr initializer_{};
    std::optional<expression> condition_{};
    std::optional<expression> increment_{};
    stmt_ptr body_{};
    uint32_t line_{};
    uint32_t column_{};

    for_stmt(stmt_ptr init, std::optional<expression> cond,
        std::optional<expression> inc, stmt_ptr body,
        uint32_t line, uint32_t column)
        : initializer_(std::move(init)), condition_(std::move(cond)),
        increment_(std::move(inc)), body_(std::move(body)),
        line_(line), column_(column) {
    }
};

struct if_stmt {
    expression condition_;
    stmt_ptr then_branch_{};
    stmt_ptr else_branch_{};
    uint32_t line_{};
    uint32_t column_{};

    if_stmt(expression cond, stmt_ptr then_branch,
        stmt_ptr else_branch, uint32_t line, uint32_t column)
        : condition_(std::move(cond)), then_branch_(std::move(then_branch)),
        else_branch_(std::move(else_branch)), line_(line), column_(column) {
    }
};

struct return_stmt {
    core::token keyword_{};
    std::optional<expression> value_{};

    return_stmt(const core::token& kw, std::optional<expression> val = std::nullopt)
        : keyword_(kw), value_(std::move(val)) {
    }
};

struct func_param {
    core::type type_{};
    core::token name_{};

    func_param(core::type t, const core::token& n)
        : type_(std::move(t)), name_(n) {
    }
};

struct func_declaration {
    core::type return_type_{};
    core::token name_{};
    std::vector<func_param> params_{};
    std::unique_ptr<block_stmt> body_{};

    func_declaration(core::type ret_type, const core::token& n)
        : return_type_(std::move(ret_type)), name_(n) {
    }
};

struct statement {
    std::variant<
        expression_stmt,
        var_declaration,
        block_stmt,
        for_stmt,
        while_stmt,
        if_stmt,
        return_stmt,
        func_declaration
    > data_;

    statement() = delete;

    template <typename T>
    statement(T s) : data_(std::move(s)) {}
};

} // namespace ast