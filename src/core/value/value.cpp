// core/value/value.cpp

#include "core/value/value.hpp"

#include "core/error/error_codes.hpp"
#include "core/utils/overloaded.hpp"

#include <charconv>
#include <format>
#include <ranges>
#include <variant>

namespace {

[[noreturn]] void invalid_conversion() {
    throw core::value_error{core::error_code::invalid_conversion};
}

core::value::int_t parse_int(std::string_view text) {
    core::value::int_t result{};
    auto&& [ptr, ec] = std::from_chars(text.begin(), text.end(), result);
    if (ec != std::errc{} || ptr != text.end()) invalid_conversion();
    return result;
}

core::value::double_t parse_double(std::string_view text) {
    core::value::double_t result{};
    auto&& [ptr, ec] = std::from_chars(text.begin(), text.end(), result);
    if (ec != std::errc{} || ptr != text.end()) invalid_conversion();
    return result;
}

}  // namespace

namespace core {

value value::default_value(const core::type& t) {
    if (t.is_int()) return int_t{};
    if (t.is_double()) return double_t{};
    if (t.is_bool()) return bool_t{};
    if (t.is_string()) return string_t{};
    if (t.is_void()) return {};
    if (t.is_unknown()) return {};

    if (t.is_array()) {
        array_t elements;
        elements.reserve(t.array_size());
        for (size_t i = 0; i < t.array_size(); ++i) elements.push_back(default_value(t.element_type()));
        return elements;
    }

    if (t.is_struct()) {
        std::vector<value> fields;
        fields.reserve(t.struct_fields().size());
        for (auto&& [_, field_type] : t.struct_fields()) fields.push_back(default_value(field_type));
        return struct_t{t, std::move(fields)};
    }

    return {};
}

value value::from_string(std::string_view text, bool is_double) {
    if (is_double) return parse_double(text);
    return parse_int(text);
}

// clang-format off

value::int_t value::to_int() const {
    return data_.visit(
        overloaded{
            [](int_t v) { return v; },
            [](double_t v) { return static_cast<int_t>(v); },
            [](bool_t v) { return static_cast<int_t>(v); },
            [](const string_wrap& v) { return parse_int(*v); },
            [](const auto&) -> int_t { invalid_conversion(); }
        });
}

value::double_t value::to_double() const {
    return data_.visit(
        overloaded{
            [](int_t v) { return static_cast<double_t>(v); },
            [](double_t v) { return v; },
            [](const string_wrap& v) { return parse_double(*v); },
            [](const auto&) -> double_t { invalid_conversion(); }
        });
}

value::bool_t value::to_bool() const {
    return data_.visit(
        overloaded{
            [](int_t v) { return static_cast<bool_t>(v); },
            [](double_t v) { return static_cast<bool_t>(v); },
            [](bool_t v) { return v; },
            [](const auto&) -> bool_t { invalid_conversion(); }
        });
}

value::string_t value::to_string() const {
    auto&& to_string = [&](std::ranges::input_range auto&& cnt) {
        string_t res;
        auto&& it = cnt.cbegin(), &&end = cnt.cend();
        if (it == end) return res;
        res.reserve(cnt.size() * 2);
        res += std::format("{}", it->to_string()); ++it;
        for(; it != end; ++it) {
            res += ", ";
            res += std::format("{}", it->to_string());
        }
        return res;
    };
    return data_.visit(
        overloaded{
            [](int_t v) { return std::format("{}", v); },
            [](double_t v) { return std::format("{}", v); },
            [](bool_t v) { return std::format("{}", v ? "true" : "false"); },
            [](const string_wrap& s) { return *s; },
            [&](const array_wrap& a) { return std::format("{{}}", to_string(*a)); },
            [&](const struct_t& s) { return std::format("{{}}", to_string(s.fields_)); },
            [](std::monostate) { return std::format("void"); }
        });
}

const value::array_t& value::to_array() const {
    return data_.visit(
        overloaded{
            [](const array_wrap& arr) -> const array_t& { return *arr; },
            [](const auto&) -> const array_t& { invalid_conversion(); }
        });
}

value::array_t& value::to_array() {
    return data_.visit(
        overloaded{
            [](array_wrap& arr) -> array_t& { return *arr; },
            [](const auto&) -> array_t& { invalid_conversion(); }
        });
}

const value::struct_t& value::to_struct() const {
    return data_.visit(
        overloaded{
            [](const struct_t& st) -> const struct_t& { return st; },
            [](const auto&) -> const struct_t& { invalid_conversion(); }
        });
}

value::struct_t& value::to_struct() {
    return data_.visit(
        overloaded{
            [](struct_t& st) -> struct_t& { return st; },
            [](const auto&) -> struct_t& { invalid_conversion(); }
        });
}

bool value::is_int() const noexcept { return data_.holds<int_t>(); }
bool value::is_double() const noexcept { return data_.holds<double_t>(); }
bool value::is_bool() const noexcept { return data_.holds<bool_t>(); }
bool value::is_string() const noexcept { return data_.holds<string_wrap>(); }
bool value::is_array() const noexcept { return data_.holds<array_wrap>(); }
bool value::is_struct() const noexcept { return data_.holds<struct_t>(); }
bool value::is_void() const noexcept { return data_.holds<std::monostate>(); }
// clang-format on

}  // namespace core
