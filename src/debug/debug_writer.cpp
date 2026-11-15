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

std::string debug_writer::indent() const {
    return std::string(curr_level_ * 2, ' ');
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
    std::println(os_, "{}", std::string(60, '-'));

    for (auto&& tok : tokens)
        std::print(os_, "{:<20}{:<25}{}:{}\n", core::token_type_names[static_cast<size_t>(tok.type_)],
                   tok.lexeme_.empty() ? "(empty)" : tok.lexeme_, tok.loc_.line_, tok.loc_.column_);

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

    stmt.visit([&](const auto& s) { print(*s); });

    if (result && enabled(trace_level::execution)) std::println(os_, "  -> {}", result->to_string());
}

void debug_writer::print(const ast::expression& expr, const core::value* result) {
    if (!enabled(trace_level::ast) && !enabled(trace_level::execution)) return;

    expr.visit([&](const auto& e) { print(*e); });

    if (result && enabled(trace_level::execution)) std::println(os_, "  -> {}", result->to_string());
}

void debug_writer::print_return(std::string_view func_name, const core::value& result) {
    if (!enabled(trace_level::returns)) return;
    std::println(os_, "{} [RETURN] {} -> {}", indent(), func_name, result.to_string());
}

void debug_writer::print_var(std::string_view name, const core::value& val) {
    if (!enabled(trace_level::execution)) return;
    std::println(os_, "{} [VAR] {} = {}", indent(), name, val.to_string());
}

void debug_writer::print(const ast::expression_stmt& stmt) {
    std::println(os_, "{} [EXEC] ExpressionStmt", indent());
    auto guard = level_guard{curr_level_};
    print(stmt.expr_);
}

void debug_writer::print(const ast::var_declaration_stmt& stmt) {
    std::println(os_, "{} [EXEC] VarDeclaration: {}", indent(), stmt.name_.lexeme_);
    auto guard = level_guard{curr_level_};
    print(stmt.initializer_);
}

void debug_writer::print(const ast::block_stmt& stmt) {
    std::println(os_, "{} [EXEC] BlockStmt [{} statements]", indent(), stmt.statements_.size());
    auto guard = level_guard{curr_level_};
    for (auto&& inner : stmt.statements_) print(inner);
}

void debug_writer::print(const ast::while_stmt& stmt) {
    std::println(os_, "{} [EXEC] WhileStmt", indent());
    auto guard = level_guard{curr_level_};

    std::println(os_, "{} Condition:", indent());
    print(stmt.condition_);

    std::println(os_, "{} Body:", indent());
    print(stmt.block_);
}

void debug_writer::print(const ast::for_stmt& stmt) {
    std::println(os_, "{} [EXEC] ForStmt", indent());
    auto guard = level_guard{curr_level_};

    if (stmt.initializer_) {
        std::println(os_, "{} Initializer:", indent());
        print(*stmt.initializer_);
    }

    if (stmt.condition_) {
        std::println(os_, "{} Condition:", indent());
        print(*stmt.condition_);
    }

    if (stmt.increment_) {
        std::println(os_, "{} Increment:", indent());
        print(*stmt.increment_);
    }

    std::println(os_, "{} Body:", indent());
    print(stmt.block_);
}

void debug_writer::print(const ast::if_stmt& stmt) {
    std::println(os_, "{} [EXEC] IfStmt", indent());
    auto guard = level_guard{curr_level_};

    std::println(os_, "{} Condition:", indent());
    print(stmt.condition_);

    std::println(os_, "{} Then:", indent());
    print(stmt.then_block_);

    if (stmt.else_block_) {
        std::println(os_, "{} Else:", indent());
        print(*stmt.else_block_);
    }
}

void debug_writer::print(const ast::return_stmt& stmt) {
    if (stmt.value_) {
        std::println(os_, "{} [EXEC] ReturnStmt", indent());
        auto guard = level_guard{curr_level_};
        print(*stmt.value_);
    } else {
        std::println(os_, "{} [EXEC] ReturnStmt (void)", indent());
    }
}

void debug_writer::print(const ast::func_declaration_stmt& stmt) {
    std::println(os_, "{} [EXEC] FuncDeclaration: {}", indent(), stmt.name_.lexeme_);

    auto guard = level_guard{curr_level_};

    auto&& params = stmt.params_;
    if (params.empty()) {
        std::println(os_, "{} Params: (none)", indent());
    } else {
        auto params_str = std::string{};
        for (auto&& p : params)
            std::format_to(std::back_inserter(params_str), "{}{}", params_str.empty() ? "" : " ", p.lexeme_);
        std::println(os_, "{} Params: {}", indent(), params_str);
    }

    std::println(os_, "{} Body:", indent());
    for (auto&& inner : stmt.body_) print(inner);
}

void debug_writer::print(const ast::struct_declaration_stmt& stmt) {
    std::println(os_, "{} [EXEC] StructDeclaration: {}", indent(), stmt.type_.name_);
    auto guard = level_guard{curr_level_};

    std::println(os_, "{} Fields:", indent());
    for (auto&& name : stmt.type_.field_names_) {
        auto field_guard = level_guard{curr_level_};
        std::println(os_, "{} {}", indent(), name);
    }

    if (!stmt.methods_.empty()) {
        std::println(os_, "{} Methods:", indent());
        for (auto&& method : stmt.methods_) print(method);
    }
}

void debug_writer::print(const ast::literal_expr& expr) {
    std::println(os_, "{} Literal: {}{}", indent(), expr.value_.to_string(), expr.loc_);
}

void debug_writer::print(const ast::variable_expr& expr) {
    std::println(os_, "{} Variable: {}{}", indent(), expr.name_.lexeme_, expr.name_.loc_);
}

void debug_writer::print(const ast::binary_expr& expr) {
    std::println(os_, "{} Binary: {}{}", indent(), expr.op_.lexeme_, expr.op_.loc_);
    auto guard = level_guard{curr_level_};

    std::println(os_, "{} Left:", indent());
    print(expr.left_);

    std::println(os_, "{} Right:", indent());
    print(expr.right_);
}

void debug_writer::print(const ast::assignment_expr& expr) {
    std::println(os_, "{} Assignment: {}{}", indent(), expr.op_.lexeme_, expr.op_.loc_);
    auto guard = level_guard{curr_level_};

    std::println(os_, "{} Target:", indent());
    print(expr.target_);

    std::println(os_, "{} Value:", indent());
    print(expr.value_);
}

void debug_writer::print(const ast::unary_expr& expr) {
    std::println(os_, "{} Unary: {}{}", indent(), expr.op_.lexeme_, expr.op_.loc_);
    auto guard = level_guard{curr_level_};
    print(expr.operand_);
}

void debug_writer::print(const ast::postfix_expr& expr) {
    std::println(os_, "{} Postfix: {}{}", indent(), expr.op_.lexeme_, expr.op_.loc_);
    auto guard = level_guard{curr_level_};
    print(expr.operand_);
}

void debug_writer::print(const ast::call_expr& expr) {
    if (expr.args_.empty()) {
        std::println(os_, "{} Call: {}{} (no args)", indent(), expr.callee_.lexeme_, expr.callee_.loc_);
        return;
    }

    std::println(os_, "{} Call: {}{}", indent(), expr.callee_.lexeme_, expr.callee_.loc_);
    auto guard = level_guard{curr_level_};
    for (auto&& [pos, arg] : std::views::enumerate(expr.args_)) {
        std::print(os_, "{} Arg {}:", indent(), pos);
        print(arg);
    }
}

void debug_writer::print(const ast::method_call_expr& expr) {
    std::println(os_, "{} MethodCall: .{}{}", indent(), expr.method_.lexeme_, expr.method_.loc_);
    auto guard = level_guard{curr_level_};

    std::println(os_, "{} Receiver:", indent());
    print(expr.object_);

    if (!expr.args_.empty()) {
        std::println(os_, "{} Args:", indent());
        auto args_guard = level_guard{curr_level_};
        for (auto&& [pos, arg] : std::views::enumerate(expr.args_)) {
            std::println(os_, "{} Arg {}:", indent(), pos);
            print(arg);
        }
    }
}

void debug_writer::print(const ast::member_access_expr& expr) {
    std::println(os_, "{} MemberAccess: .{}{}", indent(), expr.member_.lexeme_, expr.member_.loc_);
    auto guard = level_guard{curr_level_};

    std::println(os_, "{} Object:", indent());
    print(expr.object_);
}

}  // namespace cint::debug
