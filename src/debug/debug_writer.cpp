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

    stmt.visit([&](const auto& s) { print(s); });

    if (result && enabled(trace_level::execution)) std::println(os_, "  -> {}", result->to_string());
}

void debug_writer::print(const ast::expression& expr, const core::value* result) {
    if (!enabled(trace_level::ast) && !enabled(trace_level::execution)) return;

    expr.visit([&](const auto& e) { print(e); });

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
    std::println(os_, "{} [EXEC] VarDeclaration: {} : {}", indent(), stmt.name_.lexeme_, type_name(stmt.type_));
    if (stmt.initializer_) {
        auto guard = level_guard{curr_level_};
        print(*stmt.initializer_);
    }
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
    std::println(os_, "{} [EXEC] FuncDeclaration: {} -> {}", indent(), stmt.type_.function_name(),
                 type_name(stmt.type_));

    auto guard = level_guard{curr_level_};

    auto&& params = stmt.type_.param_infos();
    if (params.empty()) {
        std::println(os_, "{} Params: (none)", indent());
    } else {
        auto params_str = std::string{};
        for (auto&& [name, type] : params)
            std::format_to(std::back_inserter(params_str), "{}{} : {}", params_str.empty() ? "" : " ", name,
                           type_name(type));
        std::println(os_, "{} Params: {}", indent(), params_str);
    }

    std::println(os_, "{} Body:", indent());
    auto&& block = stmt.block_.get<ast::block_stmt>();
    for (auto&& inner : block.statements_) print(inner);
}

void debug_writer::print(const ast::struct_declaration_stmt& stmt) {
    std::println(os_, "{} [EXEC] StructDeclaration: {}", indent(), stmt.type_.struct_name());
    auto guard = level_guard{curr_level_};

    std::println(os_, "{} Fields:", indent());
    for (auto&& [name, type] : stmt.type_.struct_fields()) {
        auto guard = level_guard{curr_level_};
        std::println(os_, "{} {} : {}", indent(), name, type_name(type));
    }
}

void debug_writer::print(const ast::literal_expr& expr) {
    std::println(os_, "{} Literal: {}{}", indent(), expr.value_.to_string(), location_str(expr.loc_));
}

void debug_writer::print(const ast::variable_expr& expr) {
    std::println(os_, "{} Variable: {}{}", indent(), expr.name_.lexeme_, location_str(expr.name_.loc_));
}

void debug_writer::print(const ast::binary_expr& expr) {
    std::println(os_, "{} Binary: {}{}", indent(), expr.op_.lexeme_, location_str(expr.op_.loc_));
    auto guard = level_guard{curr_level_};

    std::println(os_, "{} Left:", indent());
    print(expr.left_);

    std::println(os_, "{} Right:", indent());
    print(expr.right_);
}

void debug_writer::print(const ast::assignment_expr& expr) {
    std::println(os_, "{} Assignment: {}{}", indent(), expr.op_.lexeme_, location_str(expr.op_.loc_));
    auto guard = level_guard{curr_level_};

    std::println(os_, "{} Target:", indent());
    print(expr.target_);

    std::println(os_, "{} Value:", indent());
    print(expr.value_);
}

void debug_writer::print(const ast::unary_expr& expr) {
    std::println(os_, "{} Unary: {}{}", indent(), expr.op_.lexeme_, location_str(expr.op_.loc_));
    auto guard = level_guard{curr_level_};
    print(expr.operand_);
}

void debug_writer::print(const ast::postfix_expr& expr) {
    std::println(os_, "{} Postfix: {}{}", indent(), expr.op_.lexeme_, location_str(expr.op_.loc_));
    auto guard = level_guard{curr_level_};
    print(expr.operand_);
}

void debug_writer::print(const ast::call_expr& expr) {
    if (expr.args_.empty()) {
        std::println(os_, "{} Call: {}{} (no args)", indent(), expr.callee_.lexeme_, location_str(expr.callee_.loc_));
        return;
    }

    std::println(os_, "{} Call: {}{}", indent(), expr.callee_.lexeme_, location_str(expr.callee_.loc_));
    auto guard = level_guard{curr_level_};
    for (auto&& [pos, arg] : std::views::enumerate(expr.args_)) {
        std::print(os_, "{} Arg {}:", indent(), pos);
        print(arg);
    }
}

void debug_writer::print(const ast::initializer_list_expr& expr) {
    std::println(os_, "{} InitializerList: [{} elements]{}", indent(), expr.elements_.size(), location_str(expr.loc_));
    auto guard = level_guard{curr_level_};
    for (auto&& [pos, elem] : std::views::enumerate(expr.elements_)) {
        std::println(os_, "{} Element {}:", indent(), pos);
        print(elem);
    }
}

void debug_writer::print(const ast::index_expr& expr) {
    std::println(os_, "{} IndexExpr{}", indent(), location_str(expr.loc_));
    auto guard = level_guard{curr_level_};

    std::println(os_, "{} Object:", indent());
    print(expr.object_);

    std::println(os_, "{} Index:", indent());
    print(expr.index_);
}

void debug_writer::print(const ast::member_access_expr& expr) {
    std::println(os_, "{} MemberAccess: .{}{}", indent(), expr.member_.lexeme_, location_str(expr.member_.loc_));
    auto guard = level_guard{curr_level_};

    std::println(os_, "{} Object:", indent());
    print(expr.object_);
}

}  // namespace cint::debug
