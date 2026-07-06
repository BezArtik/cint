// core/value/value.cpp

#include "core/value/value.hpp"

#include "core/error/error_codes.hpp"
#include "core/token/token_types.hpp"
#include "core/utils/overloaded.hpp"

#include <cassert>
#include <charconv>

namespace core {

using err = core::error_code;

value::value() : data_(std::monostate{}) {}

value::value(core::type element_type, array_t elements)
    : data_(array_info{std::move(element_type), std::move(elements)}) {}

core::type value::type() const {
    return std::visit(
        overloaded{[](int_t) { return type::int_type(); }, [](double_t) { return type::double_type(); },
                   [](bool_t) { return type::bool_type(); }, [](const string_t&) { return type::string_type(); },
                   [](const array_info& a) { return type::array_type(a.element_type_, a.elements_.size()); },
                   [](std::monostate) { return type::void_type(); }},
        data_);
}

value::int_t value::to_int() const {
    if (auto i = as_int()) return *i;
    if (auto d = as_double()) return static_cast<int_t>(*d);
    if (auto s = as_string()) {
        int_t result;
        auto [ptr, ec] = std::from_chars(s->data(), s->data() + s->size(), result);
        if (ec != std::errc{} || ptr != s->data() + s->size()) throw core::interpret_error{err::invalid_conversion};
        return result;
    }
    throw core::interpret_error{err::invalid_conversion};
}

value::double_t value::to_double() const {
    if (auto i = as_int()) return static_cast<double_t>(*i);
    if (auto d = as_double()) return *d;
    if (auto s = as_string()) {
        double_t result;
        auto [ptr, ec] = std::from_chars(s->data(), s->data() + s->size(), result);
        if (ec != std::errc{} || ptr != s->data() + s->size()) throw core::interpret_error{err::invalid_conversion};
        return result;
    }
    throw core::interpret_error{err::invalid_conversion};
}

value::string_t value::to_string() const {
    return std::visit(
        overloaded{[](int_t v) { return std::to_string(v); }, [](double_t v) { return std::to_string(v); },
                   [](bool_t v) { return string_t(v ? "true" : "false"); }, [](const string_t& v) { return v; },
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
                   [](std::monostate) { return string_t("void"); }},
        data_);
}

value::bool_t value::to_bool() const {
    return std::visit(
        core::overloaded{[](int_t v) { return v != 0; }, [](double_t v) { return v != 0.0; },
                         [](bool_t v) { return v; }, [](const string_t& v) { return !v.empty(); },
                         [](const array_info& a) { return !a.elements_.empty(); },
                         [](std::monostate) -> bool_t { throw core::interpret_error{err::invalid_conversion}; }},
        data_);
}

std::optional<value::int_t> value::as_int() const noexcept {
    return as<int_t>();
}
std::optional<value::double_t> value::as_double() const noexcept {
    return as<double_t>();
}
std::optional<value::bool_t> value::as_bool() const noexcept {
    return as<bool_t>();
}
std::optional<value::string_t> value::as_string() const noexcept {
    return as<string_t>();
}
const value::array_t* value::as_array() const noexcept {
    if (auto* p = std::get_if<array_info>(&data_)) return &p->elements_;
    return nullptr;
}
value::array_t* value::as_array() noexcept {
    if (auto* p = std::get_if<array_info>(&data_)) return &p->elements_;
    return nullptr;
}

}  // namespace core
