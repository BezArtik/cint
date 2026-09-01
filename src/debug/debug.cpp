// debug/debug.cpp

#include "debug/debug.hpp"

#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/token/token_types.hpp"
#include "core/utils/overloaded.hpp"
#include "core/value/value.hpp"

#include <cstdint>
#include <format>
#include <string>

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

void print(const debug_writer& writer, const ast::literal_expr& e, uint32_t level) {
    if (!writer.enabled(trace_level::ast)) return;
    writer.emit(indent_str(level) + "Literal: " + e.value_.to_string() + location_str(e.loc_) + "\n");
}

void print(const debug_writer& writer, const ast::variable_expr& e, uint32_t level) {
    if (!writer.enabled(trace_level::ast)) return;
    writer.emit(indent_str(level) + "Variable: " + std::string{e.name_.lexeme_} + location_str(e.name_.loc_) + "\n");
}

void print(const debug_writer& writer, const ast::binary_expr& e, uint32_t level) {
    if (!writer.enabled(trace_level::ast)) return;
    writer.emit(indent_str(level) + "Binary: " + std::string{e.op_.lexeme_} + location_str(e.op_.loc_) + "\n");
    writer.emit(indent_str(level + 1) + "Left:\n");
    print(writer, e.left_, level + 2);
    writer.emit(indent_str(level + 1) + "Right:\n");
    print(writer, e.right_, level + 2);
}

void print(const debug_writer& writer, const ast::assignment_expr& e, uint32_t level) {
    if (!writer.enabled(trace_level::ast)) return;
    writer.emit(indent_str(level) + "Assignment: " + std::string{e.op_.lexeme_} + location_str(e.op_.loc_) + "\n");
    writer.emit(indent_str(level + 1) + "Target:\n");
    print(writer, e.target_, level + 2);
    writer.emit(indent_str(level + 1) + "Value:\n");
    print(writer, e.value_, level + 2);
}

void print(const debug_writer& writer, const ast::unary_expr& e, uint32_t level) {
    if (!writer.enabled(trace_level::ast)) return;
    writer.emit(indent_str(level) + "Unary: " + std::string{e.op_.lexeme_} + location_str(e.op_.loc_) + "\n");
    print(writer, e.operand_, level + 1);
}

void print(const debug_writer& writer, const ast::postfix_expr& e, uint32_t level) {
    if (!writer.enabled(trace_level::ast)) return;
    writer.emit(indent_str(level) + "Postfix: " + std::string{e.op_.lexeme_} + location_str(e.op_.loc_) + "\n");
    print(writer, e.operand_, level + 1);
}

void print(const debug_writer& writer, const ast::call_expr& e, uint32_t level) {
    if (!writer.enabled(trace_level::ast)) return;
    auto&& msg = indent_str(level) + "Call: " + std::string{e.callee_.lexeme_} + location_str(e.callee_.loc_);
    if (e.args_.empty()) {
        writer.emit(msg + " (no args)\n");
        return;
    }
    writer.emit(msg + "\n");
    for (size_t i = 0; i < e.args_.size(); ++i) {
        writer.emit(indent_str(level + 1) + "Arg " + std::to_string(i) + ":\n");
        print(writer, e.args_[i], level + 2);
    }
}

void print(const debug_writer& writer, const ast::initializer_list_expr& e, uint32_t level) {
    if (!writer.enabled(trace_level::ast)) return;
    writer.emit(indent_str(level) + "InitializerList: [" + std::to_string(e.elements_.size()) + " elements]" +
                location_str(e.loc_) + "\n");
    for (size_t i = 0; i < e.elements_.size(); ++i) {
        writer.emit(indent_str(level + 1) + "Element " + std::to_string(i) + ":\n");
        print(writer, e.elements_[i], level + 2);
    }
}

void print(const debug_writer& writer, const ast::index_expr& e, uint32_t level) {
    if (!writer.enabled(trace_level::ast)) return;
    writer.emit(indent_str(level) + "IndexExpr" + location_str(e.loc_) + "\n");
    writer.emit(indent_str(level + 1) + "Object:\n");
    print(writer, e.object_, level + 2);
    writer.emit(indent_str(level + 1) + "Index:\n");
    print(writer, e.index_, level + 2);
}

void print(const debug_writer& writer, const ast::member_access_expr& e, uint32_t level) {
    if (!writer.enabled(trace_level::ast)) return;
    writer.emit(indent_str(level) + "MemberAccess: ." + std::string{e.member_.lexeme_} + location_str(e.member_.loc_) +
                "\n");
    writer.emit(indent_str(level + 1) + "Object:\n");
    print(writer, e.object_, level + 2);
}

void print(const debug_writer& writer, const ast::expression_stmt& s, uint32_t level) {
    if (!writer.enabled(trace_level::ast) && !writer.enabled(trace_level::execution)) return;
    writer.emit(indent_str(level) + "[EXEC] ExpressionStmt\n");
    print(writer, s.expr_, level + 1);
}

void print(const debug_writer& writer, const ast::var_declaration_stmt& s, uint32_t level) {
    if (!writer.enabled(trace_level::ast) && !writer.enabled(trace_level::execution)) return;
    writer.emit(indent_str(level) + "[EXEC] VarDeclaration: " + std::string{s.name_.lexeme_} + " : " +
                type_name(s.type_));
    if (s.initializer_) {
        writer.emit(" =\n");
        print(writer, *s.initializer_, level + 1);
    } else {
        writer.emit("\n");
    }
}

void print(const debug_writer& writer, const ast::block_stmt& s, uint32_t level) {
    if (!writer.enabled(trace_level::ast) && !writer.enabled(trace_level::execution)) return;
    writer.emit(indent_str(level) + "[EXEC] BlockStmt [" + std::to_string(s.statements_.size()) + " statements]\n");
    for (auto&& inner : s.statements_) print(writer, inner, level + 1);
}

void print(const debug_writer& writer, const ast::while_stmt& s, uint32_t level) {
    if (!writer.enabled(trace_level::ast) && !writer.enabled(trace_level::execution)) return;
    writer.emit(indent_str(level) + "[EXEC] WhileStmt\n");
    writer.emit(indent_str(level + 1) + "Condition:\n");
    print(writer, s.condition_, level + 2);
    writer.emit(indent_str(level + 1) + "Body:\n");
    print(writer, s.block_, level + 2);
}

void print(const debug_writer& writer, const ast::for_stmt& s, uint32_t level) {
    if (!writer.enabled(trace_level::ast) && !writer.enabled(trace_level::execution)) return;
    writer.emit(indent_str(level) + "[EXEC] ForStmt\n");
    if (s.initializer_) {
        writer.emit(indent_str(level + 1) + "Initializer:\n");
        print(writer, *s.initializer_, level + 2);
    }
    if (s.condition_) {
        writer.emit(indent_str(level + 1) + "Condition:\n");
        print(writer, *s.condition_, level + 2);
    }
    if (s.increment_) {
        writer.emit(indent_str(level + 1) + "Increment:\n");
        print(writer, *s.increment_, level + 2);
    }
    writer.emit(indent_str(level + 1) + "Body:\n");
    print(writer, s.block_, level + 2);
}

void print(const debug_writer& writer, const ast::if_stmt& s, uint32_t level) {
    if (!writer.enabled(trace_level::ast) && !writer.enabled(trace_level::execution)) return;
    writer.emit(indent_str(level) + "[EXEC] IfStmt\n");
    writer.emit(indent_str(level + 1) + "Condition:\n");
    print(writer, s.condition_, level + 2);
    writer.emit(indent_str(level + 1) + "Then:\n");
    print(writer, s.then_block_, level + 2);
    if (s.else_block_) {
        writer.emit(indent_str(level + 1) + "Else:\n");
        print(writer, *s.else_block_, level + 2);
    }
}

void print(const debug_writer& writer, const ast::return_stmt& s, uint32_t level) {
    if (!writer.enabled(trace_level::ast) && !writer.enabled(trace_level::execution)) return;
    if (s.value_) {
        writer.emit(indent_str(level) + "[EXEC] ReturnStmt\n");
        print(writer, *s.value_, level + 1);
    } else {
        writer.emit(indent_str(level) + "[EXEC] ReturnStmt (void)\n");
    }
}

void print(const debug_writer& writer, const ast::func_declaration_stmt& s, uint32_t level) {
    if (!writer.enabled(trace_level::ast) && !writer.enabled(trace_level::execution)) return;
    writer.emit(indent_str(level) + "[EXEC] FuncDeclaration: " + std::string{s.type_.function_name()} + " -> " +
                type_name(s.type_) + "\n");

    auto&& params_str = indent_str(level + 1) + "Params: ";
    auto&& params = s.type_.param_infos();
    if (params.empty()) {
        params_str += "(none)";
    } else {
        for (auto&& [name, type] : params) params_str += std::string{name} + " : " + type_name(type) + " ";
    }
    writer.emit(params_str + "\n");
    writer.emit(indent_str(level + 1) + "Body:\n");

    auto&& block = s.block_.get<ast::block_stmt>();
    for (auto&& inner : block.statements_) print(writer, inner, level + 2);
}

void print(const debug_writer& writer, const ast::struct_declaration_stmt& s, uint32_t level) {
    if (!writer.enabled(trace_level::ast) && !writer.enabled(trace_level::execution)) return;
    writer.emit(indent_str(level) + "[EXEC] StructDeclaration: " + std::string{s.type_.struct_name()} + "\n");
    writer.emit(indent_str(level + 1) + "Fields:\n");
    for (auto&& [field_name, field_type] : s.type_.struct_fields()) {
        writer.emit(indent_str(level + 2) + std::string{field_name} + " : " + type_name(field_type) + "\n");
    }
}

}  // namespace

void print(const debug_writer& writer, const ast::expression& expr, uint32_t level, const core::value* eval_result) {
    if (!writer.enabled(trace_level::ast) && !writer.enabled(trace_level::execution)) return;

    expr.visit(core::overloaded{
        [&](const auto& e) { print(writer, e, level); },
    });

    if (eval_result && writer.enabled(trace_level::execution)) {
        writer.emit(indent_str(level) + "  → ");
        print_value(writer, *eval_result);
    }
}

void print(const debug_writer& writer, const ast::statement& stmt, uint32_t level, const core::value* exec_result) {
    if (!writer.enabled(trace_level::ast) && !writer.enabled(trace_level::execution)) return;

    stmt.visit(core::overloaded{
        [&](const auto& s) { print(writer, s, level); },
    });

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
        writer.emit(std::format("{:<20}{:<25}{}:{}\n", core::token_type_names[static_cast<size_t>(tok.type_)], lexeme,
                                tok.loc_.line_, tok.loc_.column_));
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
        print(writer, stmt);
        writer.emit("\n");
    }
}

}  // namespace debug
