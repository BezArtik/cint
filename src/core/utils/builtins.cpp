// core/utils/builtins.cpp

#include "core/utils/builtins.hpp"

#include "core/value/value.hpp"

#include <cmath>
#include <iostream>
#include <random>
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

static auto& generator() {
    static std::mt19937_64 gen(std::random_device{}());
    return gen;
}

value srand(std::span<const value> args) {
    generator().seed(static_cast<uint64_t>(args[0].to_int()));
    return {};
}
value rand_int(std::span<const value> args) {
    std::uniform_int_distribution dist(args[0].to_int(), args[1].to_int());
    return dist(generator());
}

value rand_dbl(std::span<const value> args) {
    std::uniform_real_distribution dist(args[0].to_double(), args[1].to_double());
    return dist(generator());
}

}  // namespace core::builtin_impl
