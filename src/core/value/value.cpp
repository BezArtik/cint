// core/value/value.cpp


#include "core/value/value.hpp"
#include "core/token/token_types.hpp"
#include "core/error/error_codes.hpp"
#include "core/utils/overloaded.hpp"
#include <charconv>
#include <cassert>
#include <limits>


namespace core {

using err = core::error_code;

value::value() : data_(std::monostate{}) {}

value::value(core::type element_type, array_t elements)
    : data_(array_info{ std::move(element_type), std::move(elements) }) {
}

core::type value::type() const {
    return std::visit(overloaded{
        [](int_t)               { return type::int_type(); },
        [](double_t)            { return type::double_type(); },
        [](bool_t)              { return type::bool_type(); },
        [](const string_t&)     { return type::string_type(); },
        [](const array_info& a) { return type::array_type(a.element_type_, a.elements_.size()); },
        [](std::monostate)      { return type::void_type(); }
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
    return std::visit(overloaded{
        [](int_t v)             { return std::to_string(v); },
        [](double_t v)          { return std::to_string(v); },
        [](bool_t v)            { return string_t(v ? "true" : "false"); },
        [](const string_t& v)   { return v; },
        [](const array_info& a) {
            string_t result = "{";
            result.reserve(a.elements_.size() * 16);
            for (size_t i = 0; i < a.elements_.size(); ++i) {
                if (i > 0) result += ", ";
                result += a.elements_[i].to_string();
            }
            result += "}";
            return result;
        },
        [](std::monostate)      { return string_t("void"); }
        }, data_);
}

value::bool_t value::to_bool() const {
    return std::visit(core::overloaded{
        [](int_t v)                  { return v != 0; },
        [](double_t v)               { return v != 0.0; },
        [](bool_t v)                 { return v; },
        [](const string_t& v)        { return !v.empty(); },
        [](const array_info& a)      { return !a.elements_.empty(); },
        [](std::monostate) -> bool_t { throw core::interpret_error{ err::invalid_conversion }; }
        }, data_);
}

std::optional<value::int_t> value::as_int() const noexcept { return as<int_t>(); }
std::optional<value::double_t> value::as_double() const noexcept { return as<double_t>(); }
std::optional<value::bool_t> value::as_bool() const noexcept { return as<bool_t>();}
std::optional<value::string_t> value::as_string() const noexcept { return as<string_t>();}
const value::array_t* value::as_array() const noexcept {
    if (auto* p = std::get_if<array_info>(&data_)) return &p->elements_;
    return nullptr;
}
value::array_t* value::as_array() noexcept {
    if (auto* p = std::get_if<array_info>(&data_)) return &p->elements_;
    return nullptr;
}

size_t value::array_size() const {
    assert(std::holds_alternative<array_info>(data_));
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
    assert(type().is_int() && other.type().is_int()); 
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
    assert(type().is_bool() && other.type().is_bool()); 
    return value(to_bool() && other.to_bool());
}

value value::or_op(const value& other) const {
    assert(type().is_bool() && other.type().is_bool()); 
    return value(to_bool() || other.to_bool());
}

value value::not_op() const {
    return value(!to_bool());
}


} // namespace runtime
