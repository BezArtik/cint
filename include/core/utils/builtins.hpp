// core/utils/builtins.hpp

#pragma once
#include "core/token/type.hpp"

#include <span>
#include <string_view>
#include <vector>

namespace core {

class value;

using builtin_fn_ptr = value (*)(std::span<const value>);

struct builtin_def {
    std::string_view name_;
    type return_type_;
    std::vector<type> param_types_;
    builtin_fn_ptr impl_;
};

namespace builtin_impl {
value print_int(std::span<const value> args);
value print_dbl(std::span<const value> args);
value print_bool(std::span<const value> args);
value print_str(std::span<const value> args);
value input(std::span<const value> args);
value sqrt(std::span<const value> args);
value sin(std::span<const value> args);
value exp(std::span<const value> args);
value dtoi(std::span<const value> args);
value stoi(std::span<const value> args);
value itod(std::span<const value> args);
value stod(std::span<const value> args);
value rand_int(std::span<const value> args);
value rand_dbl(std::span<const value> args);
value srand(std::span<const value> args);
}  // namespace builtin_impl

inline const std::array builtins = {
    builtin_def{"print_int", type::void_type(), {type::int_type()}, builtin_impl::print_int},
    builtin_def{"print_dbl", type::void_type(), {type::double_type()}, builtin_impl::print_dbl},
    builtin_def{"print_bool", type::void_type(), {type::bool_type()}, builtin_impl::print_bool},
    builtin_def{"print_str", type::void_type(), {type::string_type()}, builtin_impl::print_str},

    builtin_def{"input", type::string_type(), {}, builtin_impl::input},

    builtin_def{"sqrt", type::double_type(), {type::double_type()}, builtin_impl::sqrt},
    builtin_def{"sin", type::double_type(), {type::double_type()}, builtin_impl::sin},
    builtin_def{"exp", type::double_type(), {type::double_type()}, builtin_impl::exp},

    builtin_def{"dtoi", type::int_type(), {type::double_type()}, builtin_impl::dtoi},
    builtin_def{"stoi", type::int_type(), {type::string_type()}, builtin_impl::stoi},

    builtin_def{"itod", type::double_type(), {type::int_type()}, builtin_impl::itod},
    builtin_def{"stod", type::double_type(), {type::string_type()}, builtin_impl::stod},

    builtin_def{"rand_int", type::int_type(), {type::int_type(), type::int_type()}, builtin_impl::rand_int},
    builtin_def{"rand_dbl", type::double_type(), {type::double_type(), type::double_type()}, builtin_impl::rand_dbl},
    builtin_def{"srand", type::void_type(), {type::int_type()}, builtin_impl::srand}};

}  // namespace core
