// ast/statement.hpp
#pragma once

#include "ast/node.hpp"
#include "core/memory/arena.hpp"
#include "core/token/token.hpp"

#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>

namespace cint::ast {

struct expression_stmt {
    expression expr_;
};

struct var_declaration_stmt {
    core::token name_;
    std::uint32_t slot_;
    expression initializer_;
};

struct block_stmt {
    stmt_list statements_;
    core::location loc_;
};

struct while_stmt {
    expression condition_;
    statement block_;
    core::location loc_;
};

struct for_stmt {
    std::optional<statement> initializer_;
    std::optional<expression> condition_;
    std::optional<expression> increment_;
    statement block_;
    core::location loc_;
};

struct if_stmt {
    expression condition_;
    statement then_block_;
    std::optional<statement> else_block_;
    core::location loc_;
};

struct return_stmt {
    std::optional<expression> value_;
    core::location loc_;
};

struct func_declaration_stmt {
    core::token name_;
    core::arena_vector<core::token> params_;
    stmt_list body_;
    std::uint32_t max_slots_;
    core::location loc_;
};

struct struct_declaration_stmt {
    std::string_view name_;
    core::arena_vector<std::string_view> field_names_;
    core::arena_vector<ast::func_declaration_stmt> methods_;
    core::location loc_;
};

template <typename Stmt, typename... Args>
[[nodiscard]] statement make_stmt(core::arena& arena, Args&&... args) {
    return {core::make_object<Stmt>(arena, std::forward<Args>(args)...)};
}

}  // namespace cint::ast
