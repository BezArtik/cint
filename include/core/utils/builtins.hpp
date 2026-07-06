// core/utils/builtins.hpp

#pragma once
#include "core/token/token_types.hpp"

#include <string_view>
#include <vector>

namespace core {

class value;

using builtin_fn_ptr = value (*)(const std::vector<value>&);

struct builtin_def {
    std::string_view name_;
    type return_type_;
    std::vector<type> param_types_;
    builtin_fn_ptr impl_;
};

namespace builtin_impl {
value print_int(const std::vector<value>& args);
value print_dbl(const std::vector<value>& args);
value print_bool(const std::vector<value>& args);
value print_str(const std::vector<value>& args);
value print_newline(const std::vector<value>& args);
value input(const std::vector<value>& args);
value sqrt(const std::vector<value>& args);
value sin(const std::vector<value>& args);
value exp(const std::vector<value>& args);
value dtoi(const std::vector<value>& args);
value stoi(const std::vector<value>& args);
value itod(const std::vector<value>& args);
value stod(const std::vector<value>& args);
}  // namespace builtin_impl

inline const std::array builtins = {
    builtin_def{"print_int", type::void_type(), {type::int_type()}, builtin_impl::print_int},
    builtin_def{"print_dbl", type::void_type(), {type::double_type()}, builtin_impl::print_dbl},
    builtin_def{"print_bool", type::void_type(), {type::bool_type()}, builtin_impl::print_bool},
    builtin_def{"print_str", type::void_type(), {type::string_type()}, builtin_impl::print_str},
    builtin_def{"print", type::void_type(), {}, builtin_impl::print_newline},

    builtin_def{"input", type::string_type(), {}, builtin_impl::input},

    builtin_def{"sqrt", type::double_type(), {type::double_type()}, builtin_impl::sqrt},
    builtin_def{"sin", type::double_type(), {type::double_type()}, builtin_impl::sin},
    builtin_def{"exp", type::double_type(), {type::double_type()}, builtin_impl::exp},

    builtin_def{"dtoi", type::int_type(), {type::double_type()}, builtin_impl::dtoi},
    builtin_def{"stoi", type::int_type(), {type::string_type()}, builtin_impl::stoi},

    builtin_def{"itod", type::double_type(), {type::int_type()}, builtin_impl::itod},
    builtin_def{"stod", type::double_type(), {type::string_type()}, builtin_impl::stod},
};

}  // namespace core
