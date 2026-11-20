// debug/debug_writer.hpp
#pragma once

#include "ast/node.hpp"
#include "core/token/token.hpp"
#include "core/value/value.hpp"

#include <cstdint>
#include <iostream>
#include <span>
#include <string_view>

namespace cint::debug {

enum class trace_level : std::uint8_t {
    none = 0,

    tokens = 1 << 0,
    ast = 1 << 1,
    execution = 1 << 2,
    returns = 1 << 3,

    all = tokens | ast | execution | returns
};

inline trace_level operator|(trace_level a, trace_level b) noexcept {
    return static_cast<trace_level>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
}

inline trace_level operator&(trace_level a, trace_level b) noexcept {
    return static_cast<trace_level>(static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b));
}

inline bool has_level(trace_level mask, trace_level level) noexcept {
    return (mask & level) != trace_level::none;
}

class debug_writer {
public:
    debug_writer(trace_level mask, std::ostream& os = std::cerr) : mask_{mask}, os_{os} {}

    [[nodiscard]] bool enabled(trace_level level) const noexcept;

    void print_tokens(std::span<const core::token> tokens);
    void print_ast(std::span<const ast::statement> statements);
    void print(const ast::statement& stmt, const core::value* result = nullptr);
    void print(const ast::expression& expr, const core::value* result = nullptr);
    void print_return(std::string_view func_name, const core::value& result);
    void print_var(std::string_view name, const core::value& val);

private:
    void print(const ast::expression_stmt& stmt);
    void print(const ast::var_declaration_stmt& stmt);
    void print(const ast::block_stmt& stmt);
    void print(const ast::while_stmt& stmt);
    void print(const ast::for_stmt& stmt);
    void print(const ast::if_stmt& stmt);
    void print(const ast::return_stmt& stmt);
    void print(const ast::func_declaration_stmt& stmt);
    void print(const ast::struct_declaration_stmt& stmt);

    void print(const ast::literal_expr& expr);
    void print(const ast::variable_expr& expr);
    void print(const ast::binary_expr& expr);
    void print(const ast::assignment_expr& expr);
    void print(const ast::unary_expr& expr);
    void print(const ast::postfix_expr& expr);
    void print(const ast::call_expr& expr);
    void print(const ast::method_call_expr& expr);
    void print(const ast::member_access_expr& expr);

    std::uint32_t indent() const noexcept;

    trace_level mask_;
    std::ostream& os_;
    std::uint32_t curr_level_{};
};

}  // namespace cint::debug
