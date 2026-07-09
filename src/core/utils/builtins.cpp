// core/utils/builtins.cpp

#include "core/utils/builtins.hpp"

#include "core/value/value.hpp"

#include <cmath>
#include <iostream>
#include <span>

namespace core::builtin_impl {

auto& out = std::cout;

value print_int(std::span<const value> args) {
    out << args[0].to_int();
    return {};
}

value print_dbl(std::span<const value> args) {
    out << args[0].to_double();
    return {};
}

value print_bool(std::span<const value> args) {
    out << args[0].to_string();
    return {};
}

value print_str(std::span<const value> args) {
    out << args[0].to_string();
    return {};
}

value print_newline(std::span<const value>) {
    out << std::endl;
    return {};
}

value input(std::span<const value>) {
    std::string line;
    std::getline(std::cin, line);
    return line;
}

value sqrt(std::span<const value> args) {
    return std::sqrt(args[0].to_double());
}

value sin(std::span<const value> args) {
    return std::sin(args[0].to_double());
}

value exp(std::span<const value> args) {
    return std::exp(args[0].to_double());
}

value dtoi(std::span<const value> args) {
    return args[0].to_int();
}

value stoi(std::span<const value> args) {
    return args[0].to_int();
}

value itod(std::span<const value> args) {
    return args[0].to_double();
}

value stod(std::span<const value> args) {
    return args[0].to_double();
}

}  // namespace core::builtin_impl
