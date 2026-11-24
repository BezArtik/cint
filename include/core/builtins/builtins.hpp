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

#define BUILTINS(X) \
  X(print)          \
  X(input)          \
  X(sqrt)           \
  X(sin)            \
  X(exp)            \
  X(rand_int)       \
  X(rand_dbl)       \
  X(srand)          \
  X(abs)            \
  X(gcd)            \
  X(to_int)         \
  X(to_double)      \
  X(to_str)

namespace detail {

#define DECLARE_BUILTIN(name) value name(std::span<const value> args);

BUILTINS(DECLARE_BUILTIN)
#undef DECLARE_BUILTIN

}  // namespace detail

inline constexpr std::array builtins = {
#define ENTRY_BUILTIN(name) builtin_def{#name, detail::name},

    BUILTINS(ENTRY_BUILTIN)
#undef ENTRY_BUILTIN
};

#undef BUILTINS

}  // namespace cint::core
