// ast/node.hpp
#pragma once
#include "core/memory/arena.hpp"
#include "core/utils/variant.hpp"

namespace cint::ast {

struct literal_expr;
struct variable_expr;
struct binary_expr;
struct assignment_expr;
struct unary_expr;
struct postfix_expr;
struct call_expr;
struct method_call_expr;
struct member_access_expr;

using expression =
    core::variant<core::arena_ptr<literal_expr>, core::arena_ptr<variable_expr>, core::arena_ptr<binary_expr>,
                  core::arena_ptr<assignment_expr>, core::arena_ptr<unary_expr>, core::arena_ptr<postfix_expr>,
                  core::arena_ptr<call_expr>, core::arena_ptr<method_call_expr>, core::arena_ptr<member_access_expr>>;

using expr_list = core::arena_vector<expression>;

struct expression_stmt;
struct var_declaration_stmt;
struct block_stmt;
struct while_stmt;
struct for_stmt;
struct if_stmt;
struct return_stmt;
struct func_declaration_stmt;
struct struct_declaration_stmt;

using statement = core::variant<core::arena_ptr<expression_stmt>, core::arena_ptr<var_declaration_stmt>,
                                core::arena_ptr<block_stmt>, core::arena_ptr<while_stmt>, core::arena_ptr<for_stmt>,
                                core::arena_ptr<if_stmt>, core::arena_ptr<return_stmt>,
                                core::arena_ptr<func_declaration_stmt>, core::arena_ptr<struct_declaration_stmt>>;

using stmt_list = core::arena_vector<statement>;

}  // namespace cint::ast
