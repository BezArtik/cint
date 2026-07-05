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
value sqrt_dbl(const std::vector<value>& args);
value sin_dbl(const std::vector<value>& args);
value to_int_from_int(const std::vector<value>& args);
value to_int_from_dbl(const std::vector<value>& args);
value to_int_from_str(const std::vector<value>& args);
value to_dbl_from_int(const std::vector<value>& args);
value to_dbl_from_dbl(const std::vector<value>& args);
value to_dbl_from_str(const std::vector<value>& args);
}  // namespace builtin_impl

inline const std::array builtins = {
    builtin_def{"print_int", type::void_type(), {type::int_type()}, builtin_impl::print_int},
    builtin_def{"print_dbl", type::void_type(), {type::double_type()}, builtin_impl::print_dbl},
    builtin_def{"print_bool", type::void_type(), {type::bool_type()}, builtin_impl::print_bool},
    builtin_def{"print_str", type::void_type(), {type::string_type()}, builtin_impl::print_str},
    builtin_def{"print", type::void_type(), {}, builtin_impl::print_newline},

    builtin_def{"input", type::string_type(), {}, builtin_impl::input},

    builtin_def{"sqrt", type::double_type(), {type::double_type()}, builtin_impl::sqrt_dbl},
    builtin_def{"sin", type::double_type(), {type::double_type()}, builtin_impl::sin_dbl},

    builtin_def{"dbl_to_int", type::int_type(), {type::double_type()}, builtin_impl::to_int_from_dbl},
    builtin_def{"str_to_int", type::int_type(), {type::string_type()}, builtin_impl::to_int_from_str},

    builtin_def{"int_to_dbl", type::double_type(), {type::int_type()}, builtin_impl::to_dbl_from_int},
    builtin_def{"str_to_dbl", type::double_type(), {type::string_type()}, builtin_impl::to_dbl_from_str},
};

}  // namespace core
