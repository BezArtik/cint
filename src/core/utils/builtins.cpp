// core/utils/builtins.cpp

#include "core/utils/builtins.hpp"

#include "core/value/value.hpp"

#include <cmath>
#include <iostream>

namespace core::builtin_impl {

value print_int(const std::vector<value>& args) {
    std::cout << args[0].to_int() << std::endl;
    return value{};
}

value print_dbl(const std::vector<value>& args) {
    std::cout << args[0].to_double() << std::endl;
    return value{};
}

value print_bool(const std::vector<value>& args) {
    std::cout << (args[0].to_bool() ? "true" : "false") << std::endl;
    return value{};
}

value print_str(const std::vector<value>& args) {
    std::cout << args[0].to_string() << std::endl;
    return value{};
}

value print_newline(const std::vector<value>&) {
    std::cout << std::endl;
    return value{};
}

value input(const std::vector<value>&) {
    value::string_t line;
    std::getline(std::cin, line);
    return value{std::move(line)};
}

value sqrt_dbl(const std::vector<value>& args) {
    return value{std::sqrt(args[0].to_double())};
}

value sin_dbl(const std::vector<value>& args) {
    return value{std::sin(args[0].to_double())};
}

value to_int_from_dbl(const std::vector<value>& args) {
    return value{static_cast<value::int_t>(args[0].to_double())};
}

value to_int_from_str(const std::vector<value>& args) {
    return value{args[0].to_int()};
}

value to_dbl_from_int(const std::vector<value>& args) {
    return value{static_cast<value::double_t>(args[0].to_int())};
}

value to_dbl_from_str(const std::vector<value>& args) {
    return value{args[0].to_double()};
}

}  // namespace core::builtin_impl
