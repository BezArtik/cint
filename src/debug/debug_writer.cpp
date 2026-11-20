// debug/debug_writer.cpp
#include "debug/debug_writer.hpp"

#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/token/token_types.hpp"

#include <format>
#include <print>
#include <ranges>
#include <string_view>

namespace cint::debug {

namespace {

struct level_guard {
    std::uint32_t& level_;

    level_guard(std::uint32_t& level) : level_{level} { ++level_; }
    ~level_guard() { --level_; }

    level_guard(const level_guard&) = delete;
    level_guard& operator=(const level_guard&) = delete;
};

}  // namespace

std::uint32_t debug_writer::indent() const noexcept {
    return curr_level_ * 2;
}

bool debug_writer::enabled(trace_level level) const noexcept {
    return mask_ != trace_level::none && has_level(mask_, level);
}

void debug_writer::print_tokens(std::span<const core::token> tokens) {
    if (!enabled(trace_level::tokens)) return;

    std::print(os_,
               "\n═══════════════════════════════════════════════════════\n"
               "  LEXICAL ANALYSIS\n"
               "═══════════════════════════════════════════════════════\n\n");
    std::println(os_, "{:-<60}", "");

    for (auto&& tok : tokens) {
        const auto [type, lex, loc] = tok;
        std::println(os_, "{:<20}{:<25}{}", core::token_type_names[static_cast<std::size_t>(type)], lex, loc);
    }

    std::println(os_);
}

void debug_writer::print_ast(std::span<const ast::statement> statements) {
    if (!enabled(trace_level::ast)) return;

    std::print(os_,
               "\n═══════════════════════════════════════════════════════\n"
               "  ABSTRACT SYNTAX TREE\n"
               "═══════════════════════════════════════════════════════\n\n");

    for (auto&& stmt : statements) {
        print(stmt);
        std::println(os_);
    }
}

void debug_writer::print(const ast::statement& stmt, const core::value* result) {
    if (!enabled(trace_level::ast) && !enabled(trace_level::execution)) return;

    std::visit([&](const auto& s) { print(*s); }, stmt);

    if (result && enabled(trace_level::execution)) std::println(os_, "  -> {}", result->to_string());
}

void debug_writer::print(const ast::expression& expr, const core::value* result) {
    if (!enabled(trace_level::ast) && !enabled(trace_level::execution)) return;

    std::visit([&](const auto& e) { print(*e); }, expr);

    if (result && enabled(trace_level::execution)) std::println(os_, "  -> {}", result->to_string());
}

void debug_writer::print_return(std::string_view func_name, const core::value& result) {
    if (!enabled(trace_level::returns)) return;
    std::println(os_, "{:{}}[RETURN] {} -> {}", "", indent(), func_name, result.to_string());
}

void debug_writer::print_var(std::string_view name, const core::value& val) {
    if (!enabled(trace_level::execution)) return;
    std::println(os_, "{:{}}[VAR] {} = {}", "", indent(), name, val.to_string());
}

void debug_writer::print(const ast::expression_stmt& stmt) {
    std::println(os_, "{:{}}[EXEC] ExpressionStmt", "", indent());
    auto guard = level_guard{curr_level_};
    print(stmt.expr_);
}

void debug_writer::print(const ast::var_declaration_stmt& stmt) {
    std::println(os_, "{:{}}[EXEC] VarDeclaration: {}", "", indent(), stmt.name_.lexeme_);
    auto guard = level_guard{curr_level_};
    print(stmt.initializer_);
}

void debug_writer::print(const ast::block_stmt& stmt) {
    std::println(os_, "{:{}}[EXEC] BlockStmt [{} statements]", "", indent(), stmt.statements_.size());
    auto guard = level_guard{curr_level_};
    for (auto&& inner : stmt.statements_) print(inner);
}

void debug_writer::print(const ast::while_stmt& stmt) {
    const auto pad = indent();
    std::println(os_, "{:{}}[EXEC] WhileStmt", "", pad);

    auto guard = level_guard{curr_level_};
    const auto inner_pad = indent();

    std::println(os_, "{:{}}Condition:", "", inner_pad);
    print(stmt.condition_);

    std::println(os_, "{:{}}Body:", "", inner_pad);
    print(stmt.block_);
}

void debug_writer::print(const ast::for_stmt& stmt) {
    const auto pad = indent();
    std::println(os_, "{:{}}[EXEC] ForStmt", "", pad);

    auto guard = level_guard{curr_level_};
    const auto inner_pad = indent();

    if (stmt.initializer_) {
        std::println(os_, "{:{}}Initializer:", "", inner_pad);
        print(*stmt.initializer_);
    }

    if (stmt.condition_) {
        std::println(os_, "{:{}}Condition:", "", inner_pad);
        print(*stmt.condition_);
    }

    if (stmt.increment_) {
        std::println(os_, "{:{}}Increment:", "", inner_pad);
        print(*stmt.increment_);
    }

    std::println(os_, "{:{}}Body:", "", inner_pad);
    print(stmt.block_);
}

void debug_writer::print(const ast::if_stmt& stmt) {
    const auto pad = indent();
    std::println(os_, "{:{}}[EXEC] IfStmt", "", pad);

    auto guard = level_guard{curr_level_};
    const auto inner_pad = indent();

    std::println(os_, "{:{}}Condition:", "", inner_pad);
    print(stmt.condition_);

    std::println(os_, "{:{}}Then:", "", inner_pad);
    print(stmt.then_block_);

    if (stmt.else_block_) {
        std::println(os_, "{:{}}Else:", "", inner_pad);
        print(*stmt.else_block_);
    }
}

void debug_writer::print(const ast::return_stmt& stmt) {
    const auto pad = indent();
    if (stmt.value_) {
        std::println(os_, "{:{}}[EXEC] ReturnStmt", "", pad);
        auto guard = level_guard{curr_level_};
        print(*stmt.value_);
    } else {
        std::println(os_, "{:{}}[EXEC] ReturnStmt (void)", "", pad);
    }
}

void debug_writer::print(const ast::func_declaration_stmt& stmt) {
    const auto pad = indent();
    std::println(os_, "{:{}}[EXEC] FuncDeclaration: {}", "", pad, stmt.name_.lexeme_);

    auto guard = level_guard{curr_level_};
    const auto inner_pad = indent();

    auto&& params = stmt.params_;
    if (params.empty()) {
        std::println(os_, "{:{}}Params: (none)", "", inner_pad);
    } else {
        std::print(os_, "{:{}}Params:", "", inner_pad);
        for (auto&& p : params) std::print(os_, " {}", p.lexeme_);
        std::println(os_);
    }

    std::println(os_, "{:{}}Body:", "", inner_pad);
    for (auto&& inner : stmt.body_) print(inner);
}

void debug_writer::print(const ast::struct_declaration_stmt& stmt) {
    const auto pad = indent();
    std::println(os_, "{:{}}[EXEC] StructDeclaration: {}", "", pad, stmt.type_.name_);

    auto guard = level_guard{curr_level_};
    const auto inner_pad = indent();

    std::println(os_, "{:{}}Fields:", "", inner_pad);
    {
        auto field_guard = level_guard{curr_level_};
        const auto field_pad = indent();
        for (auto&& name : stmt.type_.field_names_) std::println(os_, "{:{}}{}", "", field_pad, name);
    }

    if (!stmt.methods_.empty()) {
        std::println(os_, "{:{}}Methods:", "", inner_pad);
        for (auto&& method : stmt.methods_) print(method);
    }
}

void debug_writer::print(const ast::literal_expr& expr) {
    std::println(os_, "{:{}}Literal: {}{}", "", indent(), expr.value_.to_string(), expr.loc_);
}

void debug_writer::print(const ast::variable_expr& expr) {
    std::println(os_, "{:{}}Variable: {}{}", "", indent(), expr.name_.lexeme_, expr.name_.loc_);
}

void debug_writer::print(const ast::binary_expr& expr) {
    const auto pad = indent();
    std::println(os_, "{:{}}Binary: {}{}", "", pad, expr.op_.lexeme_, expr.op_.loc_);

    auto guard = level_guard{curr_level_};
    const auto inner_pad = indent();

    std::println(os_, "{:{}}Left:", "", inner_pad);
    print(expr.left_);

    std::println(os_, "{:{}}Right:", "", inner_pad);
    print(expr.right_);
}

void debug_writer::print(const ast::assignment_expr& expr) {
    const auto pad = indent();
    std::println(os_, "{:{}}Assignment: {}{}", "", pad, expr.op_.lexeme_, expr.op_.loc_);

    auto guard = level_guard{curr_level_};
    const auto inner_pad = indent();

    std::println(os_, "{:{}}Target:", "", inner_pad);
    print(expr.target_);

    std::println(os_, "{:{}}Value:", "", inner_pad);
    print(expr.value_);
}

void debug_writer::print(const ast::unary_expr& expr) {
    const auto pad = indent();
    std::println(os_, "{:{}}Unary: {}{}", "", pad, expr.op_.lexeme_, expr.op_.loc_);
    auto guard = level_guard{curr_level_};
    print(expr.operand_);
}

void debug_writer::print(const ast::postfix_expr& expr) {
    const auto pad = indent();
    std::println(os_, "{:{}}Postfix: {}{}", "", pad, expr.op_.lexeme_, expr.op_.loc_);
    auto guard = level_guard{curr_level_};
    print(expr.operand_);
}

void debug_writer::print(const ast::call_expr& expr) {
    const auto pad = indent();
    if (expr.args_.empty()) {
        std::println(os_, "{:{}}Call: {}{} (no args)", "", pad, expr.callee_.lexeme_, expr.callee_.loc_);
        return;
    }

    std::println(os_, "{:{}}Call: {}{}", "", pad, expr.callee_.lexeme_, expr.callee_.loc_);
    auto guard = level_guard{curr_level_};
    const auto inner_pad = indent();

    for (auto&& [pos, arg] : std::views::enumerate(expr.args_)) {
        std::println(os_, "{:{}}Arg {}:", "", inner_pad, pos);
        print(arg);
    }
}

void debug_writer::print(const ast::method_call_expr& expr) {
    const auto pad = indent();
    std::println(os_, "{:{}}MethodCall: .{}{}", "", pad, expr.method_.lexeme_, expr.method_.loc_);

    auto guard = level_guard{curr_level_};
    const auto inner_pad = indent();

    std::println(os_, "{:{}}Receiver:", "", inner_pad);
    print(expr.object_);

    if (!expr.args_.empty()) {
        std::println(os_, "{:{}}Args:", "", inner_pad);
        auto args_guard = level_guard{curr_level_};
        const auto arg_pad = indent();
        for (auto&& [pos, arg] : std::views::enumerate(expr.args_)) {
            std::println(os_, "{:{}}Arg {}:", "", arg_pad, pos);
            print(arg);
        }
    }
}

void debug_writer::print(const ast::member_access_expr& expr) {
    const auto pad = indent();
    std::println(os_, "{:{}}MemberAccess: .{}{}", "", pad, expr.member_.lexeme_, expr.member_.loc_);

    auto guard = level_guard{curr_level_};
    const auto inner_pad = indent();

    std::println(os_, "{:{}}Object:", "", inner_pad);
    print(expr.object_);
}

}  // namespace cint::debug
