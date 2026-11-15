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

namespace detail {

value print(std::span<const value> args);
value input(std::span<const value> args);
value sqrt(std::span<const value> args);
value sin(std::span<const value> args);
value exp(std::span<const value> args);
value rand_int(std::span<const value> args);
value rand_dbl(std::span<const value> args);
value srand(std::span<const value> args);
value abs(std::span<const value> args);
value gcd(std::span<const value> args);

}  // namespace detail

// clang-format off
inline constexpr std::array builtins = {
    builtin_def{"print", detail::print},       
    builtin_def{"input", detail::input},
    builtin_def{"sqrt", detail::sqrt},         
    builtin_def{"sin", detail::sin},
    builtin_def{"exp", detail::exp},                  
    builtin_def{"rand_int", detail::rand_int},
    builtin_def{"rand_dbl", detail::rand_dbl}, 
    builtin_def{"srand", detail::srand},
    builtin_def{"abs", detail::abs},
    builtin_def{"gcd", detail::gcd}
};
// clang-format on

}  // namespace cint::core
