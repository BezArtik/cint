// core/value/value.cpp

#include "core/value/value.hpp"

#include "core/error/error_codes.hpp"
#include "core/token/token_types.hpp"
#include "core/utils/overloaded.hpp"

#include <cassert>
#include <charconv>

namespace core {

value::value() : data_(std::monostate{}) {}

// clang-format off
core::type value::type() const {
    return visit(
        overloaded{
            [](int_t) { return type::int_type(); },
            [](double_t) { return type::double_type(); },
            [](bool_t) { return type::bool_type(); },
            [](const string_t&) { return type::string_type(); },
            [](const array_t& a) {
                if (a.empty() || a->empty()) return type::array_type(type::unknown_type(), 0);
                return type::array_type(a->front().type(), a->size());
            },
            [](std::monostate) { return type::void_type(); }
        },
        data_);
}

value::int_t value::to_int() const {
    if (auto* i = as<int_t>()) return *i;
    if (auto* d = as<double_t>()) return static_cast<int_t>(*d);
    if (auto* s = as<string_t>()) {
        int_t result;
        auto [_, ec] = std::from_chars(s->get().data(), s->get().data() + s->get().size(), result);
        if (ec != std::errc{}) throw core::interpret_error{error_code::invalid_conversion};
        return result;
    }
    throw core::interpret_error{error_code::invalid_conversion};
}

value::double_t value::to_double() const {
    if (auto* i = as<int_t>()) return static_cast<double_t>(*i);
    if (auto* d = as<double_t>()) return *d;
    if (auto* s = as<string_t>()) {
        double_t result;
        auto [_, ec] = std::from_chars(s->get().data(), s->get().data() + s->get().size(), result);
        if (ec != std::errc{}) throw core::interpret_error{error_code::invalid_conversion};
        return result;
    }
    throw core::interpret_error{error_code::invalid_conversion};
}

value::bool_t value::to_bool() const {
    return visit(
        overloaded{
            [](int_t v) { return v != 0; },
            [](double_t v) { return v != 0.0; },
            [](bool_t v) { return v; },
            [](const string_t& s) { return !s->empty(); },
            [](const array_t& a) { return !a->empty(); },
            [](std::monostate) -> bool_t { throw core::interpret_error{error_code::invalid_conversion}; }
        },
        data_);
}

std::string value::to_string() const {
    return visit(
        overloaded{
            [](int_t v) { return std::to_string(v); },
            [](double_t v) { return std::to_string(v); },
            [](bool_t v) { return std::string(v ? "true" : "false"); },
            [](const string_t& s) { return s.get(); },
            [](const array_t& a) {
                std::string result = "{";
                result.reserve(a->size() * 16);
                for (size_t i = 0; i < a->size(); ++i) {
                    if (i > 0) result += ", ";
                    result += (*a)[i].to_string();
                }
                result += "}";
                return result;
            },
            [](std::monostate) { return std::string("void"); }
        },
        data_);
}

bool value::is_int() const noexcept { return std::holds_alternative<int_t>(data_); }
bool value::is_double() const noexcept { return std::holds_alternative<double_t>(data_); }
bool value::is_bool() const noexcept { return std::holds_alternative<bool_t>(data_); }
bool value::is_string() const noexcept { return std::holds_alternative<string_t>(data_); }
bool value::is_array() const noexcept { return std::holds_alternative<array_t>(data_); }
bool value::is_void() const noexcept { return std::holds_alternative<std::monostate>(data_); }
// clang-format on

}  // namespace core
