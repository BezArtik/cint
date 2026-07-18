// debug/debug.hpp

#pragma once

#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/token/token.hpp"
#include "core/value/value.hpp"
#include "debug/debug_writer.hpp"

#include <cstdint>
#include <span>
#include <string_view>

namespace debug {

void print_tokens(const debug_writer& writer, std::span<const core::token> tokens);
void print_ast(const debug_writer& writer, std::span<const ast::stmt_ptr> statements);
void print_statement(const debug_writer& writer, const ast::statement& stmt, uint32_t level = 0,
                     const core::value* exec_result = nullptr);
void print_expression(const debug_writer& writer, const ast::expression& expr, uint32_t level = 0,
                      const core::value* eval_result = nullptr);
void print_value(const debug_writer& writer, const core::value& val, uint32_t indent = 0);
void print_call(const debug_writer& writer, const ast::call_expr& expr, std::span<const core::value> args,
                uint32_t level = 0);
void print_return(const debug_writer& writer, std::string_view func_name, const core::value& result,
                  uint32_t level = 0);

}  // namespace debug
