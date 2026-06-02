// core/utils/builtins.cpp


#include "core/utils/builtins.hpp"
#include "runtime/value.hpp"
#include <cmath>
#include <iostream>
#include <sstream>

namespace core::builtin_impl {

runtime::value print(const std::vector<runtime::value>& args) {
    std::ostringstream oss;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) oss << " ";
        oss << args[i].to_string();
    }
    std::cout << oss.str() << std::endl;
    return runtime::value{};
}

runtime::value input(const std::vector<runtime::value>&) {
    std::string line;
    std::getline(std::cin, line);
    return runtime::value{ std::move(line) };
}

runtime::value sqrt(const std::vector<runtime::value>& args) {
    return runtime::value{ std::sqrt(args[0].to_double()) };
}

runtime::value sin(const std::vector<runtime::value>& args) {
    return runtime::value{ std::sin(args[0].to_double()) };
}

runtime::value to_int(const std::vector<runtime::value>& args) {
    const auto& a = args[0];
    if (a.type() == type::int_type()) return a;
    if (a.type() == type::double_type())
        return runtime::value{ static_cast<int64_t>(a.to_double()) };
    return runtime::value{ static_cast<int64_t>(std::stoll(a.to_string())) };
}

runtime::value to_dbl(const std::vector<runtime::value>& args) {
    const auto& a = args[0];
    if (a.type() == type::double_type()) return a;
    if (a.type() == type::int_type())
        return runtime::value{ static_cast<double>(a.to_int()) };
    return runtime::value{ std::stod(a.to_string()) };
}

} // namespace core::builtin_impl