// core/utils/builtins.cpp

#include "core/utils/builtins.hpp"

#include "core/value/value.hpp"

#include <cmath>
#include <iostream>

namespace core::builtin_impl {

value print_int(const std::vector<value>& args) {
    std::cout << args[0].to_int();
    return value{};
}

value print_dbl(const std::vector<value>& args) {
    std::cout << args[0].to_double();
    return value{};
}

value print_bool(const std::vector<value>& args) {
    std::cout << (args[0].to_bool() ? "true" : "false");
    return value{};
}

value print_str(const std::vector<value>& args) {
    std::cout << args[0].to_string();
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

value sqrt(const std::vector<value>& args) {
    return value{std::sqrt(args[0].to_double())};
}

value sin(const std::vector<value>& args) {
    return value{std::sin(args[0].to_double())};
}

value exp(const std::vector<value>& args) {
    return value{std::exp(args[0].to_double())};
}

value dtoi(const std::vector<value>& args) {
    return value{static_cast<value::int_t>(args[0].to_double())};
}

value stoi(const std::vector<value>& args) {
    return value{args[0].to_int()};
}

value itod(const std::vector<value>& args) {
    return value{static_cast<value::double_t>(args[0].to_int())};
}

value stod(const std::vector<value>& args) {
    return value{args[0].to_double()};
}

}  // namespace core::builtin_impl
