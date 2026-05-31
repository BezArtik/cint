// runtime/value.cpp


#include "runtime/value.hpp"
#include "core/token/token_types.hpp"
#include "core/error/error_codes.hpp"
#include <stdexcept>
#include <cstdint>
#include <charconv>
#include <numeric>

namespace runtime {

value::value() : data_(std::monostate{}) {}

core::type value::type() const {
    return std::visit([](auto&& arg) -> core::type {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int64_t>)          return core::type::int_type();
        else if constexpr (std::is_same_v<T, double>)      return core::type::double_type();
        else if constexpr (std::is_same_v<T, bool>)        return core::type::bool_type();
        else if constexpr (std::is_same_v<T, std::string>) return core::type::string_type();
        else return core::type::void_type();
        }, data_);
}

int64_t value::to_int() const {
    if (auto i = as_int()) return *i;
    if (auto d = as_double()) return static_cast<int64_t>(*d);
    if (auto s = as_string()) {
        int64_t result;
        auto [ptr, ec] = std::from_chars(s->data(), s->data() + s->size(), result);
        if (ec != std::errc{}) throw core::interpret_error{ core::error_code::invalid_conversion };
        return result;
    }
    throw core::interpret_error{ core::error_code::invalid_conversion };
}

double value::to_double() const {
    if (auto i = as_int()) return static_cast<double>(*i);
    if (auto d = as_double()) return *d;
    if (auto s = as_string()) {
		double result;
		auto [ptr, ec] = std::from_chars(s->data(), s->data() + s->size(), result);
		if (ec != std::errc{}) throw core::interpret_error{core::error_code::invalid_conversion};
		return result;
    }
    throw core::interpret_error{ core::error_code::invalid_conversion };
}

std::string value::to_string() const {
    return std::visit([](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int64_t>) return std::to_string(arg);
        else if constexpr (std::is_same_v<T, double>) return std::to_string(arg);
        else if constexpr (std::is_same_v<T, bool>) return arg ? "true" : "false";
        else if constexpr (std::is_same_v<T, std::string>) return arg;
        else return "void";
        }, data_);
}

std::optional<int64_t> value::as_int() const noexcept { return as<int64_t>(); }
std::optional<double> value::as_double() const noexcept { return as<double>(); }
std::optional<bool> value::as_bool() const noexcept { return as<bool>();}
std::optional<std::string> value::as_string() const noexcept { return as<std::string>();}

value value::add(const value& other) const {
    auto lt = type();
    auto rt = other.type();
    if (!lt.is_numeric() || !rt.is_numeric())
        throw core::interpret_error{ core::error_code::invalid_conversion };
    if (lt == core::type::int_type() && rt == core::type::int_type())
        return value(*as_int() + *other.as_int());
    return value(to_double() + other.to_double());
}

value value::sub(const value& other) const {
    auto lt = type();
    auto rt = other.type();
    if (!lt.is_numeric() || !rt.is_numeric())
        throw core::interpret_error{ core::error_code::invalid_conversion };
    if (lt == core::type::int_type() && rt == core::type::int_type())
        return value(*as_int() - *other.as_int());
    return value(to_double() - other.to_double());
}

value value::mul(const value& other) const {
    auto lt = type();
    auto rt = other.type();
    if (!lt.is_numeric() || !rt.is_numeric())
        throw core::interpret_error{ core::error_code::invalid_conversion };
    if (lt == core::type::int_type() && rt == core::type::int_type())
        return value(*as_int() * *other.as_int());
    return value(to_double() * other.to_double());
}

value value::div(const value& other) const {
    auto lt = type();
    auto rt = other.type();
    if (!lt.is_numeric() || !rt.is_numeric())
        throw core::interpret_error{ core::error_code::invalid_conversion };
    if (lt == core::type::int_type() && rt == core::type::int_type()) {
        if (*other.as_int() == 0) throw core::interpret_error{ core::error_code::division_by_zero };
        return value(*as_int() / *other.as_int());
    }
    double rhs = other.to_double();
    if (std::abs(rhs) < std::numeric_limits<double>::epsilon()) 
        throw core::interpret_error{core::error_code::division_by_zero};
    return value(to_double() / rhs);
}

value value::mod(const value& other) const {
    if (type() != core::type::int_type() || other.type() != core::type::int_type())
        throw core::interpret_error{ core::error_code::modulo_requires_int };
    if (*other.as_int() == 0) throw core::interpret_error{ core::error_code::modulo_by_zero };
    return value(*as_int() % *other.as_int());
}

value value::eq(const value& other) const {
    auto lt = type();
    auto rt = other.type();
    if (lt != rt) {
        if (lt.is_numeric() && rt.is_numeric())
            return value(to_double() == other.to_double());
        return value(false);
    }
    if (lt == core::type::int_type())    return value(*as_int() == *other.as_int());
    if (lt == core::type::double_type()) return value(*as_double() == *other.as_double());
    if (lt == core::type::bool_type())   return value(*as_bool() == *other.as_bool());
    if (lt == core::type::string_type()) return value(*as_string() == *other.as_string());
    return value(false);
}

value value::neq(const value& other) const {
    return eq(other).not_op();
}

value value::lt(const value& other) const {
    auto lt = type();
    auto rt = other.type();
    if (!lt.is_numeric() || !rt.is_numeric())
        throw core::interpret_error{ core::error_code::comparison_requires_numeric };
    if (lt == core::type::int_type() && rt == core::type::int_type())
        return value(*as_int() < *other.as_int());
    return value(to_double() < other.to_double());
}

value value::le(const value& other) const {
    return lt(other).or_op(eq(other));
}

value value::gt(const value& other) const {
    return le(other).not_op();
}

value value::ge(const value& other) const {
    return lt(other).not_op();
}

value value::and_op(const value& other) const {
    if (auto b1 = as_bool())
        if (auto b2 = other.as_bool()) return value(*b1 && *b2);
    throw core::interpret_error{ core::error_code::logical_requires_bool };
}

value value::or_op(const value& other) const {
    if (auto b1 = as_bool())
        if (auto b2 = other.as_bool()) return value(*b1 || *b2);
    throw core::interpret_error{ core::error_code::logical_requires_bool };
}

value value::not_op() const {
    if (auto b = as_bool()) return value(!*b);
    throw core::interpret_error{ core::error_code::logical_requires_bool };
}


} // namespace runtime