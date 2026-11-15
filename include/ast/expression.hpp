// ast/expression.hpp
#pragma once

#include "ast/node.hpp"
#include "core/memory/arena.hpp"
#include "core/token/token.hpp"
#include "core/value/value.hpp"

#include <cstdint>

namespace cint::ast {

struct literal_expr {
    core::value value_;
    core::location loc_;
};

struct variable_expr {
    core::token name_;
    std::uint32_t slot_;
};

struct binary_expr {
    expression left_;
    core::token op_;
    expression right_;
};

struct assignment_expr {
    expression target_;
    core::token op_;
    expression value_;
};

struct unary_expr {
    core::token op_;
    expression operand_;
};

struct postfix_expr {
    expression operand_;
    core::token op_;
};

struct call_expr {
    core::token callee_;
    expr_list args_;
};

struct method_call_expr {
    expression object_;
    core::token method_;
    expr_list args_;
};

struct member_access_expr {
    expression object_;
    core::token member_;
};

template <typename T, typename... Args>
[[nodiscard]] expression make_expr(core::arena& arena, Args&&... args) {
    return {core::make_arena<T>(arena, std::forward<Args>(args)...)};
}

}  // namespace cint::ast
