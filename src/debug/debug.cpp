// debug/debug.cpp

#include "debug/debug.hpp"

#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/token/token_types.hpp"
#include "core/utils/arena.hpp"
#include "core/utils/overloaded.hpp"
#include "core/value/value.hpp"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>

namespace debug {

namespace {

std::string indent_str(uint32_t level) {
    return std::string(static_cast<size_t>(level) * 2, ' ');
}

void print_literal(const ast::literal_expr& e, uint32_t level) {
    std::cerr << indent_str(level) << "Literal: " << e.value_.lexeme_ << " [line " << e.loc_.line_ << ":"
              << e.loc_.column_ << "]\n";
}

void print_variable(const ast::variable_expr& e, uint32_t level) {
    std::cerr << indent_str(level) << "Variable: " << e.name_.lexeme_ << " [line " << e.loc_.line_ << ":"
              << e.loc_.column_ << "]\n";
}

void print_binary(const core::arena_ptr<ast::binary_expr>& e, uint32_t level) {
    std::cerr << indent_str(level) << "Binary: " << e->op_.lexeme_ << " [line " << e->loc_.line_ << ":"
              << e->loc_.column_ << "]\n";

    std::cerr << indent_str(level + 1) << "Left:\n";
    print_expression(e->left_, level + 2);

    std::cerr << indent_str(level + 1) << "Right:\n";
    print_expression(e->right_, level + 2);
}

void print_assignment(const core::arena_ptr<ast::assignment_expr>& e, uint32_t level) {
    std::cerr << indent_str(level) << "Assignment: " << e->op_.lexeme_ << " [line " << e->loc_.line_ << ":"
              << e->loc_.column_ << "]\n";
    std::cerr << indent_str(level + 1) << "Target:\n";
    print_expression(e->target_, level + 2);
    std::cerr << indent_str(level + 1) << "Value:\n";
    print_expression(e->value_, level + 2);
}

void print_unary(const core::arena_ptr<ast::unary_expr>& e, uint32_t level) {
    std::cerr << indent_str(level) << "Unary: " << e->op_.lexeme_ << " [line " << e->loc_.line_ << ":"
              << e->loc_.column_ << "]\n";

    print_expression(e->operand_, level + 1);
}

void print_postfix(const core::arena_ptr<ast::postfix_expr>& e, uint32_t level) {
    std::cerr << indent_str(level) << "Postfix: " << e->op_.lexeme_ << " [line " << e->loc_.line_ << ":"
              << e->loc_.column_ << "]\n";

    print_expression(e->operand_, level + 1);
}

void print_call(const core::arena_ptr<ast::call_expr>& e, uint32_t level) {
    std::cerr << indent_str(level) << "Call: " << e->callee_.lexeme_ << " [line " << e->loc_.line_ << ":"
              << e->loc_.column_ << "]";

    if (e->args_.empty()) {
        std::cerr << " (no args)\n";
        return;
    }

    std::cerr << "\n";
    for (size_t i = 0; i < e->args_.size(); ++i) {
        std::cerr << indent_str(level + 1) << "Arg " << i << ":\n";
        print_expression(e->args_[i], level + 2);
    }
}

void print_array_literal(const core::arena_ptr<ast::array_literal_expr>& e, uint32_t level) {
    std::cerr << indent_str(level) << "ArrayLiteral: [" << e->elements_.size() << " elements]" << " [line "
              << e->loc_.line_ << ":" << e->loc_.column_ << "]\n";
    for (size_t i = 0; i < e->elements_.size(); ++i) {
        std::cerr << indent_str(level + 1) << "Element " << i << ":\n";
        print_expression(e->elements_[i], level + 2);
    }
}

void print_index(const core::arena_ptr<ast::index_expr>& e, uint32_t level) {
    std::cerr << indent_str(level) << "IndexExpr: [line " << e->loc_.line_ << ":" << e->loc_.column_ << "]\n";
    std::cerr << indent_str(level + 1) << "Object:\n";
    print_expression(e->object_, level + 2);
    std::cerr << indent_str(level + 1) << "Index:\n";
    print_expression(e->index_, level + 2);
}

void print_expression_stmt(const ast::expression_stmt& s, uint32_t level) {
    std::cerr << indent_str(level) << "ExpressionStmt\n";
    print_expression(s.expr_, level + 1);
}

void print_var_declaration(const ast::var_declaration& s, uint32_t level) {
    std::cerr << indent_str(level) << "VarDeclaration: " << s.name_.lexeme_ << " : " << type_name(s.type_);

    if (s.initializer_) {
        std::cerr << " =\n";
        print_expression(*s.initializer_, level + 1);
    } else {
        std::cerr << "\n";
    }
}

void print_block(const ast::block_stmt& s, uint32_t level) {
    std::cerr << indent_str(level) << "BlockStmt [" << s.statements_.size() << " statements]\n";

    for (const auto& inner : s.statements_) print_statement(*inner, level + 1);
}

void print_while(const ast::while_stmt& s, uint32_t level) {
    std::cerr << indent_str(level) << "WhileStmt\n";
    std::cerr << indent_str(level + 1) << "Condition:\n";
    print_expression(s.condition_, level + 2);
    std::cerr << indent_str(level + 1) << "Body:\n";
    print_statement(*s.body_, level + 2);
}

void print_for(const ast::for_stmt& s, uint32_t level) {
    std::cerr << indent_str(level) << "ForStmt\n";

    if (s.initializer_) {
        std::cerr << indent_str(level + 1) << "Initializer:\n";
        print_statement(*s.initializer_, level + 2);
    }

    if (s.condition_) {
        std::cerr << indent_str(level + 1) << "Condition:\n";
        print_expression(*s.condition_, level + 2);
    }

    if (s.increment_) {
        std::cerr << indent_str(level + 1) << "Increment:\n";
        print_expression(*s.increment_, level + 2);
    }

    std::cerr << indent_str(level + 1) << "Body:\n";
    print_statement(*s.body_, level + 2);
}

void print_if(const ast::if_stmt& s, uint32_t level) {
    std::cerr << indent_str(level) << "IfStmt\n";

    std::cerr << indent_str(level + 1) << "Condition:\n";
    print_expression(s.condition_, level + 2);

    std::cerr << indent_str(level + 1) << "Then:\n";
    print_statement(*s.then_branch_, level + 2);

    if (s.else_branch_) {
        std::cerr << indent_str(level + 1) << "Else:\n";
        print_statement(*s.else_branch_, level + 2);
    }
}

void print_return(const ast::return_stmt& s, uint32_t level) {
    std::cerr << indent_str(level) << "ReturnStmt";

    if (s.value_) {
        std::cerr << "\n";
        print_expression(*s.value_, level + 1);
    } else {
        std::cerr << " (void)\n";
    }
}

void print_func_declaration(const ast::func_declaration& s, uint32_t level) {
    std::cerr << indent_str(level) << "FuncDeclaration: " << s.name_.lexeme_ << " -> " << type_name(s.return_type_)
              << "\n";

    std::cerr << indent_str(level + 1) << "Params: ";
    if (s.params_.empty()) { std::cerr << "(none)"; }
    for (const auto& p : s.params_) std::cerr << p.name_.lexeme_ << " : " << type_name(p.type_) << " ";
    std::cerr << "\n";

    std::cerr << indent_str(level + 1) << "Body:\n";
    for (const auto& inner : s.body_->statements_) print_statement(*inner, level + 2);
}

}  // namespace

const char* type_name(const core::type& t) {
    if (t.is_int()) return "int";
    if (t.is_double()) return "double";
    if (t.is_bool()) return "bool";
    if (t.is_string()) return "string";
    if (t.is_void()) return "void";
    if (t.is_function()) return "function";
    if (t.is_array()) return "array";
    if (t.is_unknown()) return "unknown";
    return "???";
}

void print_expression(const ast::expression& expr, uint32_t level) {
    core::visit(core::overloaded{
                    [level](const ast::literal_expr& e) { print_literal(e, level); },
                    [level](const ast::variable_expr& e) { print_variable(e, level); },
                    [level](const core::arena_ptr<ast::binary_expr>& e) { print_binary(e, level); },
                    [level](const core::arena_ptr<ast::assignment_expr>& e) { print_assignment(e, level); },
                    [level](const core::arena_ptr<ast::unary_expr>& e) { print_unary(e, level); },
                    [level](const core::arena_ptr<ast::postfix_expr>& e) { print_postfix(e, level); },
                    [level](const core::arena_ptr<ast::call_expr>& e) { print_call(e, level); },
                    [level](const core::arena_ptr<ast::array_literal_expr>& e) { print_array_literal(e, level); },
                    [level](const core::arena_ptr<ast::index_expr>& e) { print_index(e, level); },
                },
                expr);
}

void print_statement(const ast::statement& stmt, uint32_t level) {
    core::visit(core::overloaded{
                    [level](const ast::expression_stmt& s) { print_expression_stmt(s, level); },
                    [level](const ast::var_declaration& s) { print_var_declaration(s, level); },
                    [level](const ast::block_stmt& s) { print_block(s, level); },
                    [level](const ast::while_stmt& s) { print_while(s, level); },
                    [level](const ast::for_stmt& s) { print_for(s, level); },
                    [level](const ast::if_stmt& s) { print_if(s, level); },
                    [level](const ast::return_stmt& s) { print_return(s, level); },
                    [level](const ast::func_declaration& s) { print_func_declaration(s, level); },
                },
                stmt.data_);
}

void print_tokens(const std::vector<core::token>& tokens) {
    std::cerr << "\n";
    std::cerr << "═══════════════════════════════════════════════════════\n";
    std::cerr << "  LEXICAL ANALYSIS\n";
    std::cerr << "═══════════════════════════════════════════════════════\n\n";

    std::cerr << std::left << std::setw(20) << "Type" << std::setw(20) << "Lexeme" << "Location\n";
    std::cerr << std::string(60, '-') << "\n";

    for (const auto& tok : tokens) {
        std::string lexeme(tok.lexeme_);
        if (lexeme.empty()) lexeme = "(empty)";

        std::cerr << std::left << std::setw(20) << core::token_type_names[static_cast<size_t>(tok.type_)]
                  << std::setw(20) << lexeme << tok.loc_.line_ << ":" << tok.loc_.column_ << "\n";
    }

    std::cerr << "\n";
}

void print_ast(const std::vector<ast::stmt_ptr>& statements) {
    std::cerr << "\n";
    std::cerr << "═══════════════════════════════════════════════════════\n";
    std::cerr << "  ABSTRACT SYNTAX TREE\n";
    std::cerr << "═══════════════════════════════════════════════════════\n\n";

    for (const auto& stmt : statements) {
        print_statement(*stmt, 0);
        std::cerr << "\n";
    }
}

void print_value(const core::value& val, uint32_t indent) {
    std::cerr << indent_str(indent);

    auto t = val.type();

    if (t.is_int()) {
        std::cerr << "int: " << val.to_int() << "\n";
    } else if (t.is_double()) {
        std::cerr << "double: " << val.to_double() << "\n";
    } else if (t.is_bool()) {
        std::cerr << "bool: " << (val.to_bool() ? "true" : "false") << "\n";
    } else if (t.is_string()) {
        std::cerr << "string: \"" << val.to_string() << "\"\n";
    } else if (t.is_array()) {
        std::cerr << "array: \"" << val.to_string() << "\"\n";
    } else if (t.is_void()) {
        std::cerr << "void\n";
    } else {
        std::cerr << "unknown\n";
    }
}

void print_execution(const std::string& message, uint32_t indent) {
    std::cerr << indent_str(indent) << "[EXEC] " << message << "\n";
}

}  // namespace debug
