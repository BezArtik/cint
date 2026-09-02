#include "debug/debug_writer.hpp"

#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/token/token_types.hpp"
#include "core/utils/overloaded.hpp"

#include <format>

namespace {

std::string type_name(const core::type& t) {
    if (t.is_int()) return "int";
    if (t.is_double()) return "double";
    if (t.is_bool()) return "bool";
    if (t.is_string()) return "string";
    if (t.is_void()) return "void";
    if (t.is_function()) return "function";
    if (t.is_struct()) return std::format("struct {}", t.struct_name());
    if (t.is_array()) return std::format("{}[{}]", type_name(t.element_type()), t.array_size());
    return "???";
}

std::string location_str(core::location loc) {
    return std::format(" [line {}:{}]", loc.line_, loc.column_);
}

}  // namespace

namespace debug {

void debug_writer::emit_line(std::string_view msg) const {
    emit(std::format("{}{}\n", std::string(static_cast<size_t>(curr_level_) * 2, ' '), msg));
}

void debug_writer::print_tokens(std::span<const core::token> tokens) {
    if (!enabled(trace_level::tokens)) return;

    emit(
        "\n═══════════════════════════════════════════════════════\n"
        "  LEXICAL ANALYSIS\n"
        "═══════════════════════════════════════════════════════\n\n");
    emit(std::string(60, '-') + "\n");

    for (auto&& tok : tokens)
        emit(std::format("{:<20}{:<25}{}:{}\n", core::token_type_names[static_cast<size_t>(tok.type_)],
                         tok.lexeme_.empty() ? "(empty)" : tok.lexeme_, tok.loc_.line_, tok.loc_.column_));

    emit("\n");
}

void debug_writer::print_ast(std::span<const ast::statement> statements) {
    if (!enabled(trace_level::ast)) return;

    emit(
        "\n═══════════════════════════════════════════════════════\n"
        "  ABSTRACT SYNTAX TREE\n"
        "═══════════════════════════════════════════════════════\n\n");

    for (auto&& stmt : statements) {
        print(stmt);
        emit("\n");
    }
}

void debug_writer::print(const ast::statement& stmt, const core::value* result) {
    if (!enabled(trace_level::ast) && !enabled(trace_level::execution)) return;

    stmt.visit(core::overloaded{
        [&](const auto& s) { print(s); },
    });

    if (result && enabled(trace_level::execution)) {
        emit("  → ");
        print_value(*result);
    }
}

void debug_writer::print(const ast::expression& expr, const core::value* result) {
    if (!enabled(trace_level::ast) && !enabled(trace_level::execution)) return;

    expr.visit(core::overloaded{
        [&](const auto& e) { print(e); },
    });

    if (result && enabled(trace_level::execution)) {
        emit("  → ");
        print_value(*result);
    }
}

void debug_writer::print_value(const core::value& val) {
    emit(std::format("{}\n", val.to_string()));
}

void debug_writer::print_return(std::string_view func_name, const core::value& result) {
    if (!enabled(trace_level::returns)) return;
    emit_line(std::format("[RETURN] {} → {}", func_name, result.to_string()));
}

void debug_writer::print(const ast::expression_stmt& stmt) {
    emit_line("[EXEC] ExpressionStmt");
    level_guard guard{curr_level_};
    print(stmt.expr_);
}

void debug_writer::print(const ast::var_declaration_stmt& stmt) {
    emit_line(std::format("[EXEC] VarDeclaration: {} : {}", stmt.name_.lexeme_, type_name(stmt.type_)));
    if (stmt.initializer_) {
        level_guard guard{curr_level_};
        print(*stmt.initializer_);
    }
}

void debug_writer::print(const ast::block_stmt& stmt) {
    emit_line(std::format("[EXEC] BlockStmt [{} statements]", stmt.statements_.size()));
    level_guard guard{curr_level_};
    for (auto&& inner : stmt.statements_) print(inner);
}

void debug_writer::print(const ast::while_stmt& stmt) {
    emit_line("[EXEC] WhileStmt");
    level_guard guard{curr_level_};

    emit_line("Condition:");
    print(stmt.condition_);

    emit_line("Body:");
    print(stmt.block_);
}

void debug_writer::print(const ast::for_stmt& stmt) {
    emit_line("[EXEC] ForStmt");
    level_guard guard{curr_level_};

    if (stmt.initializer_) {
        emit_line("Initializer:");
        print(*stmt.initializer_);
    }

    if (stmt.condition_) {
        emit_line("Condition:");
        print(*stmt.condition_);
    }

    if (stmt.increment_) {
        emit_line("Increment:");
        print(*stmt.increment_);
    }

    emit_line("Body:");
    print(stmt.block_);
}

void debug_writer::print(const ast::if_stmt& stmt) {
    emit_line("[EXEC] IfStmt");
    level_guard guard{curr_level_};

    emit_line("Condition:");
    print(stmt.condition_);

    emit_line("Then:");
    print(stmt.then_block_);

    if (stmt.else_block_) {
        emit_line("Else:");
        print(*stmt.else_block_);
    }
}

void debug_writer::print(const ast::return_stmt& stmt) {
    if (stmt.value_) {
        emit_line("[EXEC] ReturnStmt");
        level_guard guard{curr_level_};
        print(*stmt.value_);
    } else {
        emit_line("[EXEC] ReturnStmt (void)");
    }
}

void debug_writer::print(const ast::func_declaration_stmt& stmt) {
    emit_line(std::format("[EXEC] FuncDeclaration: {} -> {}", stmt.type_.function_name(), type_name(stmt.type_)));

    level_guard guard{curr_level_};

    auto&& params = stmt.type_.param_infos();
    if (params.empty()) {
        emit_line("Params: (none)");
    } else {
        std::string params_str;
        for (auto&& [name, type] : params)
            std::format_to(std::back_inserter(params_str), "{}{} : {}", params_str.empty() ? "" : " ", name,
                           type_name(type));
        emit_line(std::format("Params: {}", params_str));
    }

    emit_line("Body:");
    auto&& block = stmt.block_.get<ast::block_stmt>();
    for (auto&& inner : block.statements_) print(inner);
}

void debug_writer::print(const ast::struct_declaration_stmt& stmt) {
    emit_line(std::format("[EXEC] StructDeclaration: {}", stmt.type_.struct_name()));
    level_guard guard{curr_level_};

    emit_line("Fields:");
    for (auto&& [name, type] : stmt.type_.struct_fields()) {
        level_guard field_guard{curr_level_};
        emit_line(std::format("{} : {}", name, type_name(type)));
    }
}

void debug_writer::print(const ast::literal_expr& expr) {
    emit_line(std::format("Literal: {}{}", expr.value_.to_string(), location_str(expr.loc_)));
}

void debug_writer::print(const ast::variable_expr& expr) {
    emit_line(std::format("Variable: {}{}", expr.name_.lexeme_, location_str(expr.name_.loc_)));
}

void debug_writer::print(const ast::binary_expr& expr) {
    emit_line(std::format("Binary: {}{}", expr.op_.lexeme_, location_str(expr.op_.loc_)));
    level_guard guard{curr_level_};

    emit_line("Left:");
    print(expr.left_);

    emit_line("Right:");
    print(expr.right_);
}

void debug_writer::print(const ast::assignment_expr& expr) {
    emit_line(std::format("Assignment: {}{}", expr.op_.lexeme_, location_str(expr.op_.loc_)));
    level_guard guard{curr_level_};

    emit_line("Target:");
    print(expr.target_);

    emit_line("Value:");
    print(expr.value_);
}

void debug_writer::print(const ast::unary_expr& expr) {
    emit_line(std::format("Unary: {}{}", expr.op_.lexeme_, location_str(expr.op_.loc_)));
    level_guard guard{curr_level_};
    print(expr.operand_);
}

void debug_writer::print(const ast::postfix_expr& expr) {
    emit_line(std::format("Postfix: {}{}", expr.op_.lexeme_, location_str(expr.op_.loc_)));
    level_guard guard{curr_level_};
    print(expr.operand_);
}

void debug_writer::print(const ast::call_expr& expr) {
    if (expr.args_.empty()) {
        emit_line(std::format("Call: {}{} (no args)", expr.callee_.lexeme_, location_str(expr.callee_.loc_)));
        return;
    }

    emit_line(std::format("Call: {}{}", expr.callee_.lexeme_, location_str(expr.callee_.loc_)));
    level_guard guard{curr_level_};
    for (size_t i = 0; i < expr.args_.size(); ++i) {
        emit_line(std::format("Arg {}:", i));
        print(expr.args_[i]);
    }
}

void debug_writer::print(const ast::initializer_list_expr& expr) {
    emit_line(std::format("InitializerList: [{} elements]{}", expr.elements_.size(), location_str(expr.loc_)));
    level_guard guard{curr_level_};
    for (size_t i = 0; i < expr.elements_.size(); ++i) {
        emit_line(std::format("Element {}:", i));
        print(expr.elements_[i]);
    }
}

void debug_writer::print(const ast::index_expr& expr) {
    emit_line(std::format("IndexExpr{}", location_str(expr.loc_)));
    level_guard guard{curr_level_};

    emit_line("Object:");
    print(expr.object_);

    emit_line("Index:");
    print(expr.index_);
}

void debug_writer::print(const ast::member_access_expr& expr) {
    emit_line(std::format("MemberAccess: .{}{}", expr.member_.lexeme_, location_str(expr.member_.loc_)));
    level_guard guard{curr_level_};

    emit_line("Object:");
    print(expr.object_);
}

}  // namespace debug
