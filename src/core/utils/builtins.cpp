// core/utils/builtins.cpp


#include "core/utils/builtins.hpp"
#include "core/value/value.hpp"
#include <cmath>
#include <iostream>
#include <sstream>

namespace core::builtin_impl {

value print(const std::vector<value>& args) {
    std::ostringstream oss;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) oss << " ";
        oss << args[i].to_string();
    }
    std::cout << oss.str() << std::endl;
    return value{};
}

value input(const std::vector<value>&) {
    value::string_t line;
    std::getline(std::cin, line);
    return value{ std::move(line) };
}

value sqrt(const std::vector<value>& args) {
    return value{ std::sqrt(args[0].to_double()) };
}

value sin(const std::vector<value>& args) {
    return value{ std::sin(args[0].to_double()) };
}

value to_int(const std::vector<value>& args) {
    const auto& a = args[0];
    if (a.type() == type::int_type()) return a;
    if (a.type() == type::double_type())
        return value{ static_cast<value::int_t>(a.to_double()) };
    return value{ static_cast<value::int_t>(std::stoll(a.to_string())) };
}

value to_dbl(const std::vector<value>& args) {
    const auto& a = args[0];
    if (a.type() == type::double_type()) return a;
    if (a.type() == type::int_type())
        return value{ static_cast<value::double_t>(a.to_int()) };
    return value{ std::stod(a.to_string()) };
}

} // namespace core::builtin_impl