// core/value.cpp


#include "core/value/value.hpp"
#include "core/token/token_types.hpp"
#include "core/error/error_codes.hpp"
#include <stdexcept>
#include <cstdint>
#include <charconv>
#include <numeric>
#include <iostream>

namespace core {

using err = core::error_code;

value::value() : data_(std::monostate{}) {}

core::type value::type() const {
    return std::visit([](auto&& arg) -> core::type {
        using T = std::decay_t<decltype(arg)>;
        using t = core::type;
        if constexpr (std::is_same_v<T, int_t>)            return t::int_type();
        else if constexpr (std::is_same_v<T, double_t>)    return t::double_type();
        else if constexpr (std::is_same_v<T, bool_t>)      return t::bool_type();
        else if constexpr (std::is_same_v<T, string_t>)    return t::string_type();
		else if constexpr (std::is_same_v<T, array_t>)     return t::array_type(t::unknown_type(), arg.size());
        else return t::void_type();
        }, data_);
}

value::int_t value::to_int() const {
    if (auto i = as_int()) return *i;
    if (auto d = as_double()) return static_cast<int_t>(*d);
    if (auto s = as_string()) {
        int_t result;
        auto [ptr, ec] = std::from_chars(s->data(), s->data() + s->size(), result);
        if (ec != std::errc{}) throw core::interpret_error{ err::invalid_conversion };
        return result;
    }
    throw core::interpret_error{ err::invalid_conversion };
}

value::double_t value::to_double() const {
    if (auto i = as_int()) return static_cast<double_t>(*i);
    if (auto d = as_double()) return *d;
    if (auto s = as_string()) {
		double_t result;
		auto [ptr, ec] = std::from_chars(s->data(), s->data() + s->size(), result);
		if (ec != std::errc{}) throw core::interpret_error{err::invalid_conversion};
		return result;
    }
    throw core::interpret_error{ err::invalid_conversion };
}

value::string_t value::to_string() const {
    return std::visit([](auto&& arg) -> string_t {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int_t>) return std::to_string(arg);
        else if constexpr (std::is_same_v<T, double_t>) return std::to_string(arg);
        else if constexpr (std::is_same_v<T, bool_t>) return arg ? "true" : "false";
        else if constexpr (std::is_same_v<T, string_t>) return arg;
        else if constexpr (std::is_same_v<T, array_t>) {
            string_t result = "{";
            result.reserve(arg.size() * 16);
            for (size_t i = 0; i < arg.size(); ++i) {
                if (i > 0) result += ", ";
                result += arg[i].to_string();
            }
            result += "}";
            return result;
        }
        else return "void";
        }, data_);
}

std::optional<value::int_t> value::as_int() const noexcept { return as<int_t>(); }
std::optional<value::double_t> value::as_double() const noexcept { return as<double_t>(); }
std::optional<value::bool_t> value::as_bool() const noexcept { return as<bool_t>();}
std::optional<value::string_t> value::as_string() const noexcept { return as<string_t>();}
std::optional<value::array_t> value::as_array() const noexcept { return as<array_t>(); }
value::array_t* value::as_array_mut() noexcept { return std::get_if<array_t>(&data_); }
std::optional<value::array_t> value::take_array() noexcept {
	if (auto* arr = as_array_mut()) {
		return std::move(*arr);
	}
	return std::nullopt;
}

size_t value::array_size() const { return std::get<array_t>(data_).size(); }

value value::add(const value& other) const {
    auto lt = type();
    auto rt = other.type();
    if (!lt.is_numeric() || !rt.is_numeric())
        throw core::interpret_error{ err::invalid_conversion };
    if (lt.is_int() && rt.is_int())
        return value(*as_int() + *other.as_int());
    return value(to_double() + other.to_double());
}

value value::sub(const value& other) const {
    auto lt = type();
    auto rt = other.type();
    if (!lt.is_numeric() || !rt.is_numeric())
        throw core::interpret_error{ err::invalid_conversion };
    if (lt.is_int() && rt.is_int())
        return value(*as_int() - *other.as_int());
    return value(to_double() - other.to_double());
}

value value::mul(const value& other) const {
    auto lt = type();
    auto rt = other.type();
    if (!lt.is_numeric() || !rt.is_numeric())
        throw core::interpret_error{ err::invalid_conversion };
    if (lt.is_int() && rt.is_int())
        return value(*as_int() * *other.as_int());
    return value(to_double() * other.to_double());
}

value value::div(const value& other) const {
    auto lt = type();
    auto rt = other.type();
    if (!lt.is_numeric() || !rt.is_numeric())
        throw core::interpret_error{ err::invalid_conversion };
    if (lt.is_int() && rt.is_int()) {
        if (*other.as_int() == 0) throw core::interpret_error{ err::division_by_zero };
        return value(*as_int() / *other.as_int());
    }
    auto rhs = other.to_double();
    if (std::abs(rhs) < std::numeric_limits<double_t>::epsilon()) 
        throw core::interpret_error{err::division_by_zero};
    return value(to_double() / rhs);
}

value value::mod(const value& other) const {
    if (!type().is_int() || !other.type().is_int())
        throw core::interpret_error{ err::modulo_requires_int };
    if (*other.as_int() == 0) throw core::interpret_error{ err::modulo_by_zero };
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
    if (lt.is_int())    return value(*as_int() == *other.as_int());
    if (lt.is_double()) return value(*as_double() == *other.as_double());
    if (lt.is_bool())   return value(*as_bool() == *other.as_bool());
    if (lt.is_string()) return value(*as_string() == *other.as_string());
    return value(false);
}

value value::neq(const value& other) const {
    return eq(other).not_op();
}

value value::lt(const value& other) const {
    auto lt = type();
    auto rt = other.type();
    if (!lt.is_numeric() || !rt.is_numeric())
        throw core::interpret_error{ err::comparison_requires_numeric };
    if (lt.is_int() && rt.is_int())
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
    throw core::interpret_error{ err::logical_requires_bool };
}

value value::or_op(const value& other) const {
    if (auto b1 = as_bool())
        if (auto b2 = other.as_bool()) return value(*b1 || *b2);
    throw core::interpret_error{ err::logical_requires_bool };
}

value value::not_op() const {
    if (auto b = as_bool()) return value(!*b);
    throw core::interpret_error{ err::logical_requires_bool };
}


} // namespace runtime