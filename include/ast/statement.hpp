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
    core::location loc_{};

    expression_stmt(expression e, core::location loc)
        : expr_(std::move(e)), loc_( loc ) {
    }
};

struct var_declaration {
    core::type type_{};
    core::token name_{};
    std::optional<expression> initializer_{};
    core::location loc_{};

    var_declaration(core::type t, const core::token& n, 
        std::optional<expression> init, core::location loc)
        : type_(std::move(t)), name_(n), initializer_(std::move(init)), loc_(loc) {
    }
};

struct block_stmt {
    std::vector<stmt_ptr> statements_{};
    core::location loc_{};

    block_stmt() = default;
    block_stmt(std::vector<stmt_ptr> statements, core::location loc)
        : statements_(std::move(statements)), loc_(loc) {}
};

struct while_stmt {
    expression condition_;
    stmt_ptr body_{};
    core::location loc_{};

    while_stmt(expression cond, stmt_ptr body, core::location loc)
        : condition_(std::move(cond)), body_(std::move(body)), loc_(loc) {}
};

struct for_stmt {
    stmt_ptr initializer_{};
    std::optional<expression> condition_{};
    std::optional<expression> increment_{};
    stmt_ptr body_{};
    core::location loc_{};

    for_stmt(stmt_ptr init, std::optional<expression> cond,
        std::optional<expression> inc, stmt_ptr body, core::location loc)
        : initializer_(std::move(init)), condition_(std::move(cond)),
        increment_(std::move(inc)), body_(std::move(body)), loc_(loc) {}
};

struct if_stmt {
    expression condition_;
    stmt_ptr then_branch_{};
    stmt_ptr else_branch_{};
    core::location loc_{};

    if_stmt(expression cond, stmt_ptr then_branch, stmt_ptr else_branch, core::location loc)
        : condition_(std::move(cond)), then_branch_(std::move(then_branch)),
          else_branch_(std::move(else_branch)), loc_(loc) {}
};

struct return_stmt {
    core::token keyword_{};
    std::optional<expression> value_{};
    core::location loc_{};

    return_stmt(const core::token& kw, std::optional<expression> val, core::location loc)
        : keyword_(kw), value_(std::move(val)), loc_(loc) {}
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
    core::location loc_{};

    func_declaration(core::type ret_type, const core::token& n)
        : return_type_(std::move(ret_type)), name_(n), loc_(n.loc_) {}
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

template <typename Stmt, typename Loc, typename... Args>
stmt_ptr make_stmt(const Loc& loc, Args&&... args) {
    Stmt s(std::forward<Args>(args)..., loc.loc_);
    return std::make_unique<statement>(std::move(s));
}

template <typename Stmt>
stmt_ptr make_stmt(Stmt&& stmt) {
    return std::make_unique<statement>(std::forward<Stmt>(stmt));
}

} // namespace ast
