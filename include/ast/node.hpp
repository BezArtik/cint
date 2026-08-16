// ast/node.hpp

#pragma once
#include "core/utils/arena.hpp"
#include <variant>
#include <vector>

namespace ast {

/**
 * @brief Шаблонный алиас для узла AST
 *
 * Тип, которым представлены все узлы AST.
 *
 */
template <typename T>
using node = core::arena_ptr<T>;

/**
 * @brief Алгебраический тип узла AST.
 *
 * Представляет из себя std::variant из
 * core::arena_ptr (время жизни узлов управляется 
 * ареной).
 *
 */
template <typename... Args>
using variant = std::variant<node<Args>...>;

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
using statement = variant<expression_stmt, var_declaration_stmt,
                          block_stmt, while_stmt,
                          for_stmt, if_stmt, return_stmt, 
                          func_declaration_stmt, struct_declaration_stmt>;

/// Список инструкций.
using stmt_list = std::pmr::vector<statement>;

/**
 * @brief Тип выражения.
 *
 * @see make_expr()
 */
using expression = variant<literal_expr, variable_expr, 
                           binary_expr, assignment_expr,
                           unary_expr, postfix_expr, 
                           call_expr, initializer_list_expr, 
                           index_expr, member_access_expr>;

/// Список выражений.
using expr_list = std::pmr::vector<expression>;

} // namespace ast
