// core/token/token.hpp
#pragma once

#include "core/token/token_types.hpp"

#include <cstdint>
#include <format>
#include <string_view>

namespace cint::core {

struct location {
    std::uint32_t line_;
    std::uint32_t column_;
};

struct token {
    token_type type_;
    std::string_view lexeme_;
    location loc_;
};

}  // namespace cint::core

namespace std {

template <>
struct formatter<cint::core::location> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    auto format(cint::core::location loc, std::format_context& ctx) const {
        return std::format_to(ctx.out(), " [line {}:{}]", loc.line_, loc.column_);
    }
};

}  // namespace std
