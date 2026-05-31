// core/utils/builtins.cpp


#include "core/utils/builtins.hpp"
#include "runtime/value.hpp"
#include <cmath>
#include <iostream>
#include <vector>
#include <sstream>

namespace core {

std::vector<builtin_def> builtins() {
    return {
        {
            "print",
            {
                {type::int_type()},
                {type::double_type()},
                {type::bool_type()},
                {type::string_type()},
                std::vector<core::type>{}
            },
            type::void_type(),
            [](const auto& args) {
                std::ostringstream oss;
                for (size_t i = 0; i < args.size(); ++i) {
                    if (i > 0) oss << " ";
                    oss << args[i].to_string();
                }
                std::cout << oss.str() << std::endl;
                return runtime::value();
            }
        },
        {
            "input",
            {std::vector<core::type>{}},
            type::string_type(),
            [](const auto& args) {
                std::string line;
                std::getline(std::cin, line);
                return runtime::value(line);
            }
        },
        {
            "sqrt",
            {{type::double_type()}, {type::int_type()}},
            type::double_type(),
            [](const auto& args) {
                double x = args[0].to_double();
                return runtime::value(std::sqrt(x));
            }
        },
        {
            "sin",
            {{type::double_type()}, {type::int_type()}},
            type::double_type(),
            [](const auto& args) {
                double x = args[0].to_double();
                return runtime::value(std::sin(x));
            }
        },
        {
            "to_int",
            {{type::int_type()}, {type::double_type()}, {type::string_type()}},
            type::int_type(),
            [](const auto& args) {
                const auto& a = args[0];
                if (a.type() == type::int_type()) return a;
                if (a.type() == type::double_type())
                    return runtime::value(static_cast<int64_t>(a.to_double()));
                return runtime::value(static_cast<int64_t>(std::stoll(a.to_string())));
            }
        },
        {
            "to_double",
            {{type::double_type()}, {type::int_type()}, {type::string_type()}},
            type::double_type(),
            [](const auto& args) {
                const auto& a = args[0];
                if (a.type() == type::double_type()) return a;
                if (a.type() == type::int_type())
                    return runtime::value(static_cast<double>(a.to_int()));
                return runtime::value(std::stod(a.to_string()));
            }
        }
    };
}

} // namespace core