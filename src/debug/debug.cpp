// debug/debug.cpp

#include "debug/debug.hpp"

#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/token/token_types.hpp"
#include "core/utils/overloaded.hpp"
#include "core/value/value.hpp"

#include <cstdint>
#include <string>
#include <format>

namespace debug {

namespace {

std::string indent_str(uint32_t level) {
    return std::string(static_cast<size_t>(level) * 2, ' ');
}

std::string type_name(const core::type& t) {
    if (t.is_int()) return "int";
    if (t.is_double()) return "double";
    if (t.is_bool()) return "bool";
    if (t.is_string()) return "string";
    if (t.is_void()) return "void";
    if (t.is_function()) return "function";
    if (t.is_struct()) return "struct " + std::string{t.struct_name()};
    if (t.is_array()) return type_name(t.element_type()) + "[" + std::to_string(t.array_size()) + "]";
    return "???";
}

std::string location_str(core::location loc) {
    return " [line " + std::to_string(loc.line_) + ":" + std::to_string(loc.column_) + "]";
}



void print_literal(const debug_writer& writer, const ast::literal_expr& e, uint32_t level) {
    if (!writer.enabled(trace_level::ast)) return;
    writer.emit(indent_str(level) + "Literal: " + e.value_.to_string() + location_str(e.loc_) + "\n");
}

void print_variable(const debug_writer& writer, const ast::variable_expr& e, uint32_t level) {
    if (!writer.enabled(trace_level::ast)) return;
    writer.emit(indent_str(level) + "Variable: " + std::string{e.name_.lexeme_} + location_str(e.loc_) + "\n");
}

void print_binary(const debug_writer& writer, const ast::binary_expr& e, uint32_t level) {
    if (!writer.enabled(trace_level::ast)) return;
    writer.emit(indent_str(level) + "Binary: " + std::string{e.op_.lexeme_} + location_str(e.loc_) + "\n");
    writer.emit(indent_str(level + 1) + "Left:\n");
    print_expression(writer, e.left_, level + 2);
    writer.emit(indent_str(level + 1) + "Right:\n");
    print_expression(writer, e.right_, level + 2);
}

void print_assignment(const debug_writer& writer, const ast::assignment_expr& e, uint32_t level) {
    if (!writer.enabled(trace_level::ast)) return;
    writer.emit(indent_str(level) + "Assignment: " + std::string{e.op_.lexeme_} + location_str(e.loc_) + "\n");
    writer.emit(indent_str(level + 1) + "Target:\n");
    print_expression(writer, e.target_, level + 2);
    writer.emit(indent_str(level + 1) + "Value:\n");
    print_expression(writer, e.value_, level + 2);
}

void print_unary(const debug_writer& writer, const ast::unary_expr& e, uint32_t level) {
    if (!writer.enabled(trace_level::ast)) return;
    writer.emit(indent_str(level) + "Unary: " + std::string{e.op_.lexeme_} + location_str(e.loc_) + "\n");
    print_expression(writer, e.operand_, level + 1);
}

void print_postfix(const debug_writer& writer, const ast::postfix_expr& e, uint32_t level) {
    if (!writer.enabled(trace_level::ast)) return;
    writer.emit(indent_str(level) + "Postfix: " + std::string{e.op_.lexeme_} + location_str(e.loc_) + "\n");
    print_expression(writer, e.operand_, level + 1);
}

void print_call_ast(const debug_writer& writer, const ast::call_expr& e, uint32_t level) {
    if (!writer.enabled(trace_level::ast)) return;
    auto&& msg = indent_str(level) + "Call: " + std::string{e.callee_.lexeme_} + location_str(e.loc_);
    if (e.args_.empty()) {
        writer.emit(msg + " (no args)\n");
        return;
    }
    writer.emit(msg + "\n");
    for (size_t i = 0; i < e.args_.size(); ++i) {
        writer.emit(indent_str(level + 1) + "Arg " + std::to_string(i) + ":\n");
        print_expression(writer, e.args_[i], level + 2);
    }
}

void print_initializer_list_ast(const debug_writer& writer, const ast::initializer_list_expr& e,
                                uint32_t level) {
    if (!writer.enabled(trace_level::ast)) return;
    writer.emit(indent_str(level) + "InitializerList: [" + std::to_string(e.elements_.size()) + " elements]" +
                location_str(e.loc_) + "\n");
    for (size_t i = 0; i < e.elements_.size(); ++i) {
        writer.emit(indent_str(level + 1) + "Element " + std::to_string(i) + ":\n");
        print_expression(writer, e.elements_[i], level + 2);
    }
}

void print_index_ast(const debug_writer& writer, const ast::index_expr& e, uint32_t level) {
    if (!writer.enabled(trace_level::ast)) return;
    writer.emit(indent_str(level) + "IndexExpr" + location_str(e.loc_) + "\n");
    writer.emit(indent_str(level + 1) + "Object:\n");
    print_expression(writer, e.object_, level + 2);
    writer.emit(indent_str(level + 1) + "Index:\n");
    print_expression(writer, e.index_, level + 2);
}

void print_member_access(const debug_writer& writer, const ast::member_access_expr& e, uint32_t level) {
    if (!writer.enabled(trace_level::ast)) return;
    writer.emit(indent_str(level) + "MemberAccess: ." + std::string{e.member_.lexeme_} + location_str(e.loc_) + "\n");
    writer.emit(indent_str(level + 1) + "Object:\n");
    print_expression(writer, e.object_, level + 2);
}



void print_expression_stmt_ast(const debug_writer& writer, const ast::expression_stmt& s, uint32_t level) {
    if (!writer.enabled(trace_level::ast) && !writer.enabled(trace_level::execution)) return;
    writer.emit(indent_str(level) + "[EXEC] ExpressionStmt\n");
    print_expression(writer, s.expr_, level + 1);
}

void print_var_declaration_stmt_ast(const debug_writer& writer, const ast::var_declaration_stmt& s, uint32_t level) {
    if (!writer.enabled(trace_level::ast) && !writer.enabled(trace_level::execution)) return;
    writer.emit(indent_str(level) + "[EXEC] VarDeclaration: " + std::string{s.name_.lexeme_} + " : " +
                type_name(s.type_));
    if (s.initializer_) {
        writer.emit(" =\n");
        print_expression(writer, *s.initializer_, level + 1);
    } else {
        writer.emit("\n");
    }
}

void print_block_stmt_ast(const debug_writer& writer, const ast::block_stmt& s, uint32_t level) {
    if (!writer.enabled(trace_level::ast) && !writer.enabled(trace_level::execution)) return;
    writer.emit(indent_str(level) + "[EXEC] BlockStmt [" + std::to_string(s.statements_.size()) + " statements]\n");
    for (auto&& inner : s.statements_) print_statement(writer, inner, level + 1);
}

void print_while_stmt_ast(const debug_writer& writer, const ast::while_stmt& s, uint32_t level) {
    if (!writer.enabled(trace_level::ast) && !writer.enabled(trace_level::execution)) return;
    writer.emit(indent_str(level) + "[EXEC] WhileStmt\n");
    writer.emit(indent_str(level + 1) + "Condition:\n");
    print_expression(writer, s.condition_, level + 2);
    writer.emit(indent_str(level + 1) + "Body:\n");
    print_statement(writer, s.block_, level + 2);
}

void print_for_stmt_ast(const debug_writer& writer, const ast::for_stmt& s, uint32_t level) {
    if (!writer.enabled(trace_level::ast) && !writer.enabled(trace_level::execution)) return;
    writer.emit(indent_str(level) + "[EXEC] ForStmt\n");
    if (s.initializer_) {
        writer.emit(indent_str(level + 1) + "Initializer:\n");
        print_statement(writer, *s.initializer_, level + 2);
    }
    if (s.condition_) {
        writer.emit(indent_str(level + 1) + "Condition:\n");
        print_expression(writer, *s.condition_, level + 2);
    }
    if (s.increment_) {
        writer.emit(indent_str(level + 1) + "Increment:\n");
        print_expression(writer, *s.increment_, level + 2);
    }
    writer.emit(indent_str(level + 1) + "Body:\n");
    print_statement(writer, s.block_, level + 2);
}

void print_if_stmt_ast(const debug_writer& writer, const ast::if_stmt& s, uint32_t level) {
    if (!writer.enabled(trace_level::ast) && !writer.enabled(trace_level::execution)) return;
    writer.emit(indent_str(level) + "[EXEC] IfStmt\n");
    writer.emit(indent_str(level + 1) + "Condition:\n");
    print_expression(writer, s.condition_, level + 2);
    writer.emit(indent_str(level + 1) + "Then:\n");
    print_statement(writer, s.then_block_, level + 2);
    if (s.else_block_) {
        writer.emit(indent_str(level + 1) + "Else:\n");
        print_statement(writer, *s.else_block_, level + 2);
    }
}

void print_return_stmt_ast(const debug_writer& writer, const ast::return_stmt& s, uint32_t level) {
    if (!writer.enabled(trace_level::ast) && !writer.enabled(trace_level::execution)) return;
    if (s.value_) {
        writer.emit(indent_str(level) + "[EXEC] ReturnStmt\n");
        print_expression(writer, *s.value_, level + 1);
    } else {
        writer.emit(indent_str(level) + "[EXEC] ReturnStmt (void)\n");
    }
}

void print_func_declaration_stmt_ast(const debug_writer& writer, const ast::func_declaration_stmt& s, uint32_t level) {
    if (!writer.enabled(trace_level::ast) && !writer.enabled(trace_level::execution)) return;
    writer.emit(indent_str(level) + "[EXEC] FuncDeclaration: " + std::string{s.name_.lexeme_} + " -> " +
                type_name(s.return_type_) + "\n");

    auto&& params = indent_str(level + 1) + "Params: ";
    if (s.params_.empty()) {
        params += "(none)";
    } else {
        for (auto&& p : s.params_)
            params += std::string{p.name_.lexeme_} + " : " + type_name(p.type_) + " ";
    }
    writer.emit(params + "\n");
    writer.emit(indent_str(level + 1) + "Body:\n");

    auto&& block = std::get<ast::node<ast::block_stmt>>(s.block_);
    for (auto&& inner : block->statements_) print_statement(writer, inner, level + 2);
}

void print_struct_declaration_stmt_ast(const debug_writer& writer, const ast::struct_declaration_stmt& s,
                                       uint32_t level) {
    if (!writer.enabled(trace_level::ast) && !writer.enabled(trace_level::execution)) return;
    writer.emit(indent_str(level) + "[EXEC] StructDeclaration: " + std::string{s.name_.lexeme_} + "\n");
    writer.emit(indent_str(level + 1) + "Fields:\n");
    for (auto&& [field_name, field_type] : s.type_.struct_fields()) {
        writer.emit(indent_str(level + 2) + std::string{field_name} + " : " + type_name(field_type) + "\n");
    }
}

}  // namespace

void print_expression(const debug_writer& writer, const ast::expression& expr, uint32_t level,
                      const core::value* eval_result) {
    if (!writer.enabled(trace_level::ast) && !writer.enabled(trace_level::execution)) return;

    core::visit(
        core::overloaded{
            [&](const ast::node<ast::literal_expr>& e) { print_literal(writer, *e, level); },
            [&](const ast::node<ast::variable_expr>& e) { print_variable(writer, *e, level); },
            [&](const ast::node<ast::binary_expr>& e) { print_binary(writer, *e, level); },
            [&](const ast::node<ast::assignment_expr>& e) { print_assignment(writer, *e, level); },
            [&](const ast::node<ast::unary_expr>& e) { print_unary(writer, *e, level); },
            [&](const ast::node<ast::postfix_expr>& e) { print_postfix(writer, *e, level); },
            [&](const ast::node<ast::call_expr>& e) { print_call_ast(writer, *e, level); },
            [&](const ast::node<ast::initializer_list_expr>& e) { print_initializer_list_ast(writer, *e, level); },
            [&](const ast::node<ast::index_expr>& e) { print_index_ast(writer, *e, level); },
            [&](const ast::node<ast::member_access_expr>& e) { print_member_access(writer, *e, level); }},
        expr);

    if (eval_result && writer.enabled(trace_level::execution)) {
        writer.emit(indent_str(level) + "  → ");
        print_value(writer, *eval_result);
    }
}

void print_statement(const debug_writer& writer, const ast::statement& stmt, uint32_t level,
                     const core::value* exec_result) {
    if (!writer.enabled(trace_level::ast) && !writer.enabled(trace_level::execution)) return;

    core::visit(
        core::overloaded{
            [&](const ast::node<ast::expression_stmt>& s) { print_expression_stmt_ast(writer, *s, level); },
            [&](const ast::node<ast::var_declaration_stmt>& s) { print_var_declaration_stmt_ast(writer, *s, level); },
            [&](const ast::node<ast::block_stmt>& s) { print_block_stmt_ast(writer, *s, level); },
            [&](const ast::node<ast::while_stmt>& s) { print_while_stmt_ast(writer, *s, level); },
            [&](const ast::node<ast::for_stmt>& s) { print_for_stmt_ast(writer, *s, level); },
            [&](const ast::node<ast::if_stmt>& s) { print_if_stmt_ast(writer, *s, level); },
            [&](const ast::node<ast::return_stmt>& s) { print_return_stmt_ast(writer, *s, level); },
            [&](const ast::node<ast::func_declaration_stmt>& s) { print_func_declaration_stmt_ast(writer, *s, level); },
            [&](const ast::node<ast::struct_declaration_stmt>& s) { print_struct_declaration_stmt_ast(writer, *s, level); }},
        stmt);

    if (exec_result && writer.enabled(trace_level::execution)) {
        writer.emit(indent_str(level) + "  → ");
        print_value(writer, *exec_result);
    }
}

void print_return(const debug_writer& writer, std::string_view func_name, const core::value& result, uint32_t level) {
    if (!writer.enabled(trace_level::returns)) return;
    writer.emit(indent_str(level) + " [RETURN] " + std::string{func_name} + " → " + result.to_string() + "\n");
}

void print_value(const debug_writer& writer, const core::value& val, uint32_t indent) {
    writer.emit(indent_str(indent) + val.to_string() + "\n");
}

void print_tokens(const debug_writer& writer, std::span<const core::token> tokens) {
    if (!writer.enabled(trace_level::tokens)) return;

    writer.emit(
        "\n═══════════════════════════════════════════════════════\n"
        "  LEXICAL ANALYSIS\n"
        "═══════════════════════════════════════════════════════\n\n");
    writer.emit(std::string(60, '-') + "\n");

    for (auto&& tok : tokens) {
        auto&& lexeme = tok.lexeme_.empty() ? "(empty)" : tok.lexeme_;
        writer.emit(std::format("{:<20}{:<25}{}:{}\n", core::token_type_names[static_cast<size_t>(tok.type_)],
                    lexeme, tok.loc_.line_, tok.loc_.column_));
    }
    writer.emit("\n");
}

void print_ast(const debug_writer& writer, std::span<const ast::statement> statements) {
    if (!writer.enabled(trace_level::ast)) return;

    writer.emit(
        "\n═══════════════════════════════════════════════════════\n"
        "  ABSTRACT SYNTAX TREE\n"
        "═══════════════════════════════════════════════════════\n\n");

    for (auto&& stmt : statements) {
        print_statement(writer, stmt);
        writer.emit("\n");
    }
}

}  // namespace debug
