/**
 * @file include/ast/node.hpp
 * @brief Узлы AST.
 * @ingroup AST
 *
 * @defgroup AST Абстрактное синтаксическое дерево
 * @brief Типы данных, представляющие узлы AST — выражения, инструкции, объявления.
 */

#pragma once
#include "ast/variant_wrapper.hpp"

#include <vector>

namespace ast {

// Forward declarations expressions
struct literal_expr;
struct variable_expr;
struct binary_expr;
struct assignment_expr;
struct unary_expr;
struct postfix_expr;
struct call_expr;
struct initializer_list_expr;
struct index_expr;
struct member_access_expr;

// Forward declarations statements
struct expression_stmt;
struct var_declaration_stmt;
struct block_stmt;
struct while_stmt;
struct for_stmt;
struct if_stmt;
struct return_stmt;
struct func_declaration_stmt;
struct struct_declaration_stmt;

/**
 * @brief Тип инструкции.
 *
 * @see make_stmt()
 */
using statement = variant_wrapper<expression_stmt, var_declaration_stmt, block_stmt, while_stmt, for_stmt, if_stmt,
                                  return_stmt, func_declaration_stmt, struct_declaration_stmt>;

/// Список инструкций.
using stmt_list = std::pmr::vector<statement>;

/**
 * @brief Тип выражения.
 *
 * @see make_expr()
 */
using expression = variant_wrapper<literal_expr, variable_expr, binary_expr, assignment_expr, unary_expr, postfix_expr,
                                   call_expr, initializer_list_expr, index_expr, member_access_expr>;

/// Список выражений.
using expr_list = std::pmr::vector<expression>;

}  // namespace ast
