// ast/statement.hpp

#pragma once

#include "ast/expression.hpp"
#include "core/token/token_types.hpp"
#include "core/utils/arena.hpp"

#include <algorithm>
#include <variant>
#include <vector>

namespace ast {

struct statement;

using stmt_ptr = core::arena_ptr<statement>;
using stmt_list = std::pmr::vector<stmt_ptr>;

struct expression_stmt {
    expression expr_;
    core::location loc_;

    expression_stmt(expression e, core::location loc) : expr_(std::move(e)), loc_(loc) {}
};

struct var_declaration {
    core::type type_;
    core::token name_;
    std::optional<expression> initializer_;
    core::location loc_;

    var_declaration(core::type t, const core::token& n, std::optional<expression> init, core::location loc)
        : type_(std::move(t)), name_(n), initializer_(std::move(init)), loc_(loc) {}
};

struct block_stmt {
    stmt_list statements_;
    core::location loc_;
    mutable bool has_declarations_checked_ = false;
    mutable bool has_declarations_ = false;

    block_stmt() = default;
    block_stmt(stmt_list statements, core::location loc) : statements_(std::move(statements)), loc_(loc) {}
};

struct while_stmt {
    expression condition_;
    stmt_ptr block_;
    core::location loc_;

    while_stmt(expression cond, stmt_ptr block, core::location loc)
        : condition_(std::move(cond)), block_(std::move(block)), loc_(loc) {}
};

struct for_stmt {
    stmt_ptr initializer_;
    std::optional<expression> condition_;
    std::optional<expression> increment_;
    stmt_ptr block_;
    core::location loc_;

    for_stmt(stmt_ptr init, std::optional<expression> cond, std::optional<expression> inc, stmt_ptr block,
             core::location loc)
        : initializer_(std::move(init)),
          condition_(std::move(cond)),
          increment_(std::move(inc)),
          block_(std::move(block)),
          loc_(loc) {}
};

struct if_stmt {
    expression condition_;
    stmt_ptr then_block_;
    stmt_ptr else_block_;
    core::location loc_;

    if_stmt(expression cond, stmt_ptr then_block, stmt_ptr else_block, core::location loc)
        : condition_(std::move(cond)),
          then_block_(std::move(then_block)),
          else_block_(std::move(else_block)),
          loc_(loc) {}
};

struct return_stmt {
    core::token keyword_;
    std::optional<expression> value_;
    core::location loc_;

    return_stmt(const core::token& kw, std::optional<expression> val, core::location loc)
        : keyword_(kw), value_(std::move(val)), loc_(loc) {}
};

struct func_param {
    core::type type_;
    core::token name_;

    func_param(core::type t, const core::token& n) : type_(std::move(t)), name_(n) {}
};

struct func_declaration {
    core::type return_type_;
    core::token name_;
    std::pmr::vector<func_param> params_;
    core::arena_ptr<block_stmt> block_;
    core::location loc_;

    func_declaration(core::type ret_type, const core::token& n)
        : return_type_(std::move(ret_type)), name_(n), loc_(n.loc_) {}
};

struct struct_declaration {
    core::type type_;
    core::token name_;
    core::location loc_;

    struct_declaration(core::type type, const core::token& name, core::location loc)
        : type_(std::move(type)), name_(name), loc_(loc) {}
};

struct statement {
    std::variant<expression_stmt, var_declaration, block_stmt, for_stmt, while_stmt, if_stmt, return_stmt,
                 func_declaration, struct_declaration>
        data_;

    statement() = delete;

    template <typename T>
    statement(T s) : data_(std::move(s)) {}
};

template <typename Stmt, typename... Args>
stmt_ptr make_stmt(core::arena& arena, core::location loc, Args&&... args) {
    auto* p = arena.allocate<statement>(Stmt{std::forward<Args>(args)..., loc});
    return stmt_ptr(p);
}

template <typename Stmt>
stmt_ptr make_stmt(core::arena& arena, Stmt&& stmt) {
    auto* p = arena.allocate<statement>(std::forward<Stmt>(stmt));
    return stmt_ptr(p);
}

inline bool has_declarations(const block_stmt& block) noexcept {
    if (!block.has_declarations_checked_) {
        block.has_declarations_ = std::ranges::any_of(block.statements_, [](const auto& stmt) {
            return std::holds_alternative<var_declaration>(stmt->data_) ||
                   (std::holds_alternative<block_stmt>(stmt->data_) &&
                    has_declarations(std::get<block_stmt>(stmt->data_)));
        });
        block.has_declarations_checked_ = true;
    }
    return block.has_declarations_;
}

}  // namespace ast
