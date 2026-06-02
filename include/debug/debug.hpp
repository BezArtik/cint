// debug/debug.hpp


#pragma once
#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/token/token.hpp"
#include <vector>
#include <memory>
#include <string>
#include <cstdint>


namespace runtime { class value; }

namespace debug {

void print_tokens(const std::vector<core::token>& tokens);
const char* type_name(const core::type& t);

void print_ast(const std::vector<std::unique_ptr<ast::statement>>& statements);
void print_statement(const ast::statement& stmt, uint32_t level = 0);
void print_expression(const ast::expression& expr, uint32_t level = 0);

void print_semantic_info();

void print_value(const runtime::value& val, uint32_t indent = 0);
void print_execution(const std::string& message, uint32_t indent = 0);

} // namespace debug