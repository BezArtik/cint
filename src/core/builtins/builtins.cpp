// core/builtins/builtins.cpp
#include "core/builtins/builtins.hpp"

#include "core/value/value.hpp"

#include <cmath>
#include <iostream>
#include <print>
#include <random>
#include <ranges>
#include <span>
#include <string>

namespace cint::core::detail {

value print(std::span<const value> args) {
    for (auto&& [pos, arg] : std::views::enumerate(args)) {
        if (pos) std::print(" ");
        std::print("{}", arg.to_string());
    }
    std::println();
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

namespace {

auto& generator() {
    static std::mt19937_64 gen(std::random_device{}());
    return gen;
}

}  // namespace

value srand(std::span<const value> args) {
    generator().seed(args[0].to_int());
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

value abs(std::span<const value> args) {
    return std::abs(args[0].to_int());
}

value gcd(std::span<const value> args) {
    return std::gcd(args[0].to_int(), args[1].to_int());
}

}  // namespace cint::core::detail
