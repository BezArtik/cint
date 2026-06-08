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

value::value(core::type element_type, array_t elements)
    : data_(array_info{ std::move(element_type), std::move(elements) }) {
}

core::type value::type() const {
    return std::visit([](auto&& arg) -> core::type {
        using T = std::decay_t<decltype(arg)>;
        using t = core::type;
        if constexpr (std::is_same_v<T, int_t>)            return t::int_type();
        else if constexpr (std::is_same_v<T, double_t>)    return t::double_type();
        else if constexpr (std::is_same_v<T, bool_t>)      return t::bool_type();
        else if constexpr (std::is_same_v<T, string_t>)    return t::string_type();
		else if constexpr (std::is_same_v<T, array_info>)  return t::array_type(arg.element_type_, arg.elements_.size());
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
        else if constexpr (std::is_same_v<T, array_info>) {
            string_t result = "{";
            const auto& elems = arg.elements_;
            result.reserve(elems.size() * 16);
            for (size_t i = 0; i < elems.size(); ++i) {
                if (i > 0) result += ", ";
                result += elems[i].to_string();
            }
            result += "}";
            return result;
        }
        else return "void";
        }, data_);
}

value::bool_t value::to_bool() const {
    return std::visit([](auto&& arg) -> bool_t {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int_t>)               return arg != 0;
        else if constexpr (std::is_same_v<T, double_t>)       return arg != 0.0;
        else if constexpr (std::is_same_v<T, bool_t>)         return arg;
        else if constexpr (std::is_same_v<T, string_t>)       return !arg.empty();
        else if constexpr (std::is_same_v<T, array_info>)     return !arg.elements_.empty();
        else if constexpr (std::is_same_v<T, std::monostate>) throw core::interpret_error{ err::invalid_conversion };
        }, data_);
}

std::optional<value::int_t> value::as_int() const noexcept { return as<int_t>(); }
std::optional<value::double_t> value::as_double() const noexcept { return as<double_t>(); }
std::optional<value::bool_t> value::as_bool() const noexcept { return as<bool_t>();}
std::optional<value::string_t> value::as_string() const noexcept { return as<string_t>();}
std::optional<value::array_t> value::as_array() const noexcept {
    if (auto* p = std::get_if<array_info>(&data_)) return p->elements_;
    return std::nullopt;
}
value::array_t* value::as_array_mut() noexcept {
    if (auto* p = std::get_if<array_info>(&data_)) return &p->elements_;
    return nullptr;
}
std::optional<value::array_t> value::take_array() noexcept {
    if (auto* p = std::get_if<array_info>(&data_)) return std::move(p->elements_);
    return std::nullopt;
}

size_t value::array_size() const {
    return std::get<array_info>(data_).elements_.size();
}

value value::add(const value& other) const {
    if (type().is_int() && other.type().is_int())
        return value(to_int() + other.to_int());
    return value(to_double() + other.to_double());
}

value value::sub(const value& other) const {
    if (type().is_int() && other.type().is_int())
        return value(to_int() - other.to_int());
    return value(to_double() - other.to_double());
}

value value::mul(const value& other) const {
    if (type().is_int() && other.type().is_int())
        return value(to_int() * other.to_int());
    return value(to_double() * other.to_double());
}

value value::div(const value& other) const {
    if (type().is_int() && other.type().is_int()) {
        auto rhs = other.to_int();
        if (rhs == 0) throw core::interpret_error{ err::division_by_zero };
        return value(to_int() / rhs);
    }
    auto rhs = other.to_double();
    if (std::abs(rhs) < std::numeric_limits<double_t>::epsilon()) 
        throw core::interpret_error{err::division_by_zero};
    return value(to_double() / rhs);
}

value value::mod(const value& other) const {
    auto li = as_int();
    auto ri = other.as_int();
    if (!li || !ri) throw core::interpret_error{ err::modulo_requires_int };
    if (*ri == 0) throw core::interpret_error{ err::modulo_by_zero };
    return value(*li % *ri);
}

value value::eq(const value& other) const {
    auto lt = type();
    auto rt = other.type();
    if (lt != rt) {
        if (lt.is_numeric() && rt.is_numeric())
            return value(to_double() == other.to_double());
        return value(false);
    }
    if (lt.is_int())    return value(to_int() == other.to_int());
    if (lt.is_double()) return value(to_double() == other.to_double());
    if (lt.is_bool())   return value(to_bool() == other.to_bool());
    if (lt.is_string()) return value(to_string() == other.to_string());
    return value(false);
}

value value::neq(const value& other) const {
    return eq(other).not_op();
}

value value::lt(const value& other) const {
    if (type().is_int() && other.type().is_int())
        return value(to_double() < other.to_int());
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
    return value(to_bool() && other.to_bool());
}

value value::or_op(const value& other) const {
    return value(to_bool() || other.to_bool());
}

value value::not_op() const {
    return value(!to_bool());
}


} // namespace runtime