// core/utils/builtins.cpp

#include "core/utils/builtins.hpp"

#include "core/value/value.hpp"

#include <cmath>
#include <iostream>

namespace core::builtin_impl {

auto& out = std::cout;

value print_int(const std::vector<value>& args) {
    out << args[0].to_int();
    return {};
}

value print_dbl(const std::vector<value>& args) {
    out << args[0].to_double();
    return {};
}

value print_bool(const std::vector<value>& args) {
    out << args[0].to_string();
    return {};
}

value print_str(const std::vector<value>& args) {
    out << args[0].to_string();
    return {};
}

value print_newline(const std::vector<value>&) {
    out << std::endl;
    return {};
}

value input(const std::vector<value>&) {
    std::string line;
    std::getline(std::cin, line);
    return line;
}

value sqrt(const std::vector<value>& args) {
    return std::sqrt(args[0].to_double());
}

value sin(const std::vector<value>& args) {
    return std::sin(args[0].to_double());
}

value exp(const std::vector<value>& args) {
    return std::exp(args[0].to_double());
}

value dtoi(const std::vector<value>& args) {
    return args[0].to_int();
}

value stoi(const std::vector<value>& args) {
    return args[0].to_int();
}

value itod(const std::vector<value>& args) {
    return args[0].to_double();
}

value stod(const std::vector<value>& args) {
    return args[0].to_double();
}

}  // namespace core::builtin_impl
