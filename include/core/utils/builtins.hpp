// core/utils/builtins.hpp

#pragma once
#include "core/token/token_types.hpp"
#include <string_view>
#include <vector>

namespace core {

class value;

using builtin_fn_ptr = value(*)(const std::vector<value>&);

struct builtin_overload {
    std::vector<type> param_types_;
    type return_type_;

    builtin_overload(std::vector<type> params, type ret)
        : param_types_(std::move(params)), return_type_(std::move(ret)) {}
};

struct builtin_def {
    std::string_view name_;
    std::vector<builtin_overload> overloads_;
    builtin_fn_ptr impl_;
};

namespace builtin_impl {
value print(const std::vector<value>& args);
value input(const std::vector<value>& args);
value sqrt(const std::vector<value>& args);
value sin(const std::vector<value>& args);
value to_int(const std::vector<value>& args);
value to_dbl(const std::vector<value>& args);
}

inline const std::array builtins = {
    builtin_def{
        "print",
        {{
            {{type::int_type()},    type::void_type()},
            {{type::double_type()}, type::void_type()},
            {{type::bool_type()},   type::void_type()},
            {{type::string_type()}, type::void_type()},
            {{},                     type::void_type()}
        }},
        builtin_impl::print
    },
    builtin_def{
        "input",
        {{
            {{}, type::string_type()}
        }},
        builtin_impl::input
    },
    builtin_def{
        "sqrt",
        {{
            {{type::double_type()}, type::double_type()},
            {{type::int_type()},    type::double_type()}
        }},
        builtin_impl::sqrt
    },
    builtin_def{
        "sin",
        {{
            {{type::double_type()}, type::double_type()},
            {{type::int_type()},    type::double_type()}
        }},
        builtin_impl::sin
    },
    builtin_def{
        "to_int",
        {{
            {{type::int_type()},    type::int_type()},
            {{type::double_type()}, type::int_type()},
            {{type::string_type()}, type::int_type()}
        }},
        builtin_impl::to_int
    },
    builtin_def{
        "to_double",
        {{
            {{type::double_type()}, type::double_type()},
            {{type::int_type()},    type::double_type()},
            {{type::string_type()}, type::double_type()}
        }},
        builtin_impl::to_dbl
    }
};

} // namespace core
