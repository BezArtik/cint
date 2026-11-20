// core/builtins/builtins.hpp
#pragma once

#include <array>
#include <span>
#include <string_view>

namespace cint::core {

class value;

using builtin_fn = value (*)(std::span<const value>);

struct builtin_def {
    std::string_view name_;
    builtin_fn impl_;
};

#define BUILTINS(X)        \
    X(print, print)        \
    X(input, input)        \
    X(sqrt, sqrt)          \
    X(sin, sin)            \
    X(exp, exp)            \
    X(rand_int, rand_int)  \
    X(rand_dbl, rand_dbl)  \
    X(srand, srand)        \
    X(abs, abs)            \
    X(gcd, gcd)            \
    X(int, int_cast)       \
    X(double, double_cast) \
    X(str, str_cast)

namespace detail {

#define DECLARE_BUILTIN(lang_name, fn_name) value fn_name(std::span<const value> args);

BUILTINS(DECLARE_BUILTIN)
#undef DECLARE_BUILTIN

}  // namespace detail

inline constexpr std::array builtins = {
#define ENTRY_BUILTIN(lang_name, fn_name) builtin_def{#lang_name, detail::fn_name},

    BUILTINS(ENTRY_BUILTIN)
#undef ENTRY_BUILTIN
};

#undef BUILTINS

}  // namespace cint::core
