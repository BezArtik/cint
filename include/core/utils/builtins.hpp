// core/utils/builtins.hpp

#pragma once
#include "core/token/token_types.hpp"
#include <string_view>
#include <array>
#include <vector>

namespace core {

class value;

using builtin_fn_ptr = value(*)(const std::vector<value>&);

struct builtin_overload {
    std::vector<type> param_types_;
    type return_type_;
};

struct builtin_def {
    std::string_view name_;
    std::vector<builtin_overload> overloads_;
    builtin_fn_ptr impl_;
};

namespace builtin_sig {
inline std::vector<type> sig() { return {}; }
template<typename... Types>
std::vector<type> sig(Types... types) { return { types... }; }
inline type i() { return type::int_type(); }
inline type d() { return type::double_type(); }
inline type b() { return type::bool_type(); }
inline type s() { return type::string_type(); }
inline type void_t() { return type::void_type(); }
}

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
            {builtin_sig::sig(builtin_sig::i()), builtin_sig::void_t()},
            {builtin_sig::sig(builtin_sig::d()), builtin_sig::void_t()},
            {builtin_sig::sig(builtin_sig::b()), builtin_sig::void_t()},
            {builtin_sig::sig(builtin_sig::s()), builtin_sig::void_t()},
            {builtin_sig::sig(),                 builtin_sig::void_t()}
        }},
        builtin_impl::print
    },
    builtin_def{
        "input",
        {{
            {builtin_sig::sig(), builtin_sig::s()}
        }},
        builtin_impl::input
    },
    builtin_def{
        "sqrt",
        {{
            {builtin_sig::sig(builtin_sig::d()), builtin_sig::d()},
            {builtin_sig::sig(builtin_sig::i()), builtin_sig::d()}
        }},
        builtin_impl::sqrt
    },
    builtin_def{
        "sin",
        {{
            {builtin_sig::sig(builtin_sig::d()), builtin_sig::d()},
            {builtin_sig::sig(builtin_sig::i()), builtin_sig::d()}
        }},
        builtin_impl::sin
    },
    builtin_def{
        "to_int",
        {{
            {builtin_sig::sig(builtin_sig::i()), builtin_sig::i()},
            {builtin_sig::sig(builtin_sig::d()), builtin_sig::i()},
            {builtin_sig::sig(builtin_sig::s()), builtin_sig::i()}
        }},
        builtin_impl::to_int
    },
    builtin_def{
        "to_double",
        {{
            {builtin_sig::sig(builtin_sig::d()), builtin_sig::d()},
            {builtin_sig::sig(builtin_sig::i()), builtin_sig::d()},
            {builtin_sig::sig(builtin_sig::s()), builtin_sig::d()}
        }},
        builtin_impl::to_dbl
    }
};

} // namespace core