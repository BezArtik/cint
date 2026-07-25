// core/value/value.cpp

#include "core/value/value.hpp"

#include "core/error/error_codes.hpp"
#include "core/utils/overloaded.hpp"

#include <cassert>
#include <charconv>

namespace core {

core::value value::default_value(const core::type& t) {
    using k = core::type::kind;
    switch (t.get_kind()) {
        case k::INT:
            return int_t{};
        case k::DOUBLE:
            return double_t{};
        case k::BOOL:
            return bool_t{};
        case k::STRING:
            return std::string{};
        case k::VOID:
            return {};
        case k::ARRAY: {
            std::vector<value> elements;
            elements.reserve(t.array_size());
            for (size_t i = 0; i < t.array_size(); ++i) elements.push_back(default_value(t.element_type()));
            return elements;
        }
        case k::STRUCT: {
            std::vector<value> fields;
            fields.reserve(t.struct_fields().size());
            for (const auto& [_, field_type] : t.struct_fields()) fields.push_back(default_value(field_type));
            return struct_t{t, fields};
        }
        default:
            return {};
    }
}

core::value value::convert(core::value val, const core::type& target) {
    if (val.type() == target) return val;

    if (target.is_int() && val.is_double()) return val.to_int();
    if (target.is_double() && val.is_int()) return val.to_double();
    return val;
}

value::int_t value::parse_int(std::string_view text) {
    int_t result;
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), result);
    if (ec != std::errc{} || ptr != text.data() + text.size())
        throw core::interpret_error{error_code::invalid_conversion};
    return result;
}

value::double_t value::parse_double(std::string_view text) {
    double_t result;
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), result);
    if (ec != std::errc{} || ptr != text.data() + text.size())
        throw core::interpret_error{error_code::invalid_conversion};
    return result;
}

value value::from_string(std::string_view text, bool is_double) {
    if (is_double) return parse_double(text);
    return parse_int(text);
}

// clang-format off
core::type value::type() const {
    return visit(
        overloaded{
            [](int_t) { return type::int_type(); },
            [](double_t) { return type::double_type(); },
            [](bool_t) { return type::bool_type(); },
            [](const string_t&) { return type::string_type(); },
            [](const array_t& a) {
                if (a->empty() || a->empty()) return type::array_type(type::unknown_type(), 0);
                return type::array_type(a->front().type(), a->size());
            },
            [](const struct_t& s) { return s.type_; },
            [](std::monostate) { return type::void_type(); }
        },
        data_);
}

value::int_t value::to_int() const {
    return visit(
        overloaded{
            [](int_t v) { return v; },
            [](double_t v) { return static_cast<int_t>(v); },
            [](const string_t& v) { return parse_int(*v); },
            [](const auto&) -> int_t { throw core::interpret_error{error_code::invalid_conversion}; }
        },
        data_);
}

value::double_t value::to_double() const {
    return visit(
        overloaded{
            [](int_t v) { return static_cast<double_t>(v); },
            [](double_t v) { return v; },
            [](const string_t& v) { return parse_double(*v); },
            [](const auto&) -> double_t { throw core::interpret_error{error_code::invalid_conversion}; }
        },
        data_);
}

value::bool_t value::to_bool() const {
    return visit(
        overloaded{
            [](int_t v) { return v != 0; },
            [](double_t v) { return v != 0.0; },
            [](bool_t v) { return v; },
            [](const string_t& s) { return !s->empty(); },
            [](const array_t& a) { return !a->empty(); },
            [](const auto&) -> bool_t { throw core::interpret_error{error_code::invalid_conversion}; }
        },
        data_);
}

std::string value::to_string() const {
    auto to_string_range = [&](const auto& cnt) {
        std::string res = "{";
        res.reserve(cnt.size() * 16);
        for(size_t i = 0; i < cnt.size(); ++i) {
            if (i > 0) res += ", ";
            res += cnt[i].to_string();
        }
        res += "}";
        return res;
    };
    return visit(
        overloaded{
            [](int_t v) { return std::to_string(v); },
            [](double_t v) { return std::to_string(v); },
            [](bool_t v) { return std::string(v ? "true" : "false"); },
            [](const string_t& s) { return *s; },
            [&](const array_t& a) { return to_string_range(*a); },
            [&](const struct_t& s) { return to_string_range(s.fields_); },
            [](std::monostate) { return std::string("void"); }
        },
        data_);
}

bool value::is_int() const noexcept { return std::holds_alternative<int_t>(data_); }
bool value::is_double() const noexcept { return std::holds_alternative<double_t>(data_); }
bool value::is_bool() const noexcept { return std::holds_alternative<bool_t>(data_); }
bool value::is_string() const noexcept { return std::holds_alternative<string_t>(data_); }
bool value::is_array() const noexcept { return std::holds_alternative<array_t>(data_); }
bool value::is_struct() const noexcept { return std::holds_alternative<struct_t>(data_); }
bool value::is_void() const noexcept { return std::holds_alternative<std::monostate>(data_); }
// clang-format on

}  // namespace core
