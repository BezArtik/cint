// core/value/value.cpp

#include "core/value/value.hpp"

#include "core/error/error_codes.hpp"
#include "core/utils/overloaded.hpp"

#include <charconv>

namespace core {

core::value value::default_value(const core::type& t) {
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
        std::vector<core::value> fields;
        fields.reserve(t.struct_fields().size());
        for (auto&& [_, field_type] : t.struct_fields()) fields.push_back(default_value(field_type));
        return struct_t{t, std::move(fields)};
    }

    return {};
}

value::int_t value::parse_int(std::string_view text) {
    int_t result{};
    auto&& [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), result);
    if (ec != std::errc{} || ptr != text.data() + text.size()) throw core::value_error{error_code::invalid_conversion};
    return result;
}

value::double_t value::parse_double(std::string_view text) {
    double_t result{};
    auto&& [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), result);
    if (ec != std::errc{} || ptr != text.data() + text.size()) throw core::value_error{error_code::invalid_conversion};
    return result;
}

value value::from_string(std::string_view text, bool is_double) {
    if (is_double) return parse_double(text);
    return parse_int(text);
}

// clang-format off

value::int_t value::to_int() const {
    return visit(
        overloaded{
            [](int_t v) { return v; },
            [](double_t v) { return static_cast<int_t>(v); },
            [](const std::shared_ptr<string_t>& v) { return parse_int(*v); },
            [](const auto&) -> int_t { throw core::value_error{error_code::invalid_conversion}; }
        },
        data_);
}

value::double_t value::to_double() const {
    return visit(
        overloaded{
            [](int_t v) { return static_cast<double_t>(v); },
            [](double_t v) { return v; },
            [](const std::shared_ptr<string_t>& v) { return parse_double(*v); },
            [](const auto&) -> double_t { throw core::value_error{error_code::invalid_conversion}; }
        },
        data_);
}

value::bool_t value::to_bool() const {
    return visit(
        overloaded{
            [](int_t v) { return v != 0; },
            [](double_t v) { return v != 0.0; },
            [](bool_t v) { return v; },
            [](const std::shared_ptr<string_t>& s) { return !s->empty(); },
            [](const std::shared_ptr<array_t>& a) { return !a->empty(); },
            [](const auto&) -> bool_t { throw core::value_error{error_code::invalid_conversion}; }
        },
        data_);
}

value::string_t value::to_string() const {
    auto&& to_string_range = [&](auto&& cnt) {
        string_t res{"{"};
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
            [](bool_t v) { return string_t{v ? "true" : "false"}; },
            [](const std::shared_ptr<string_t>& s) { return *s; },
            [&](const std::shared_ptr<array_t>& a) { return to_string_range(*a); },
            [&](const struct_t& s) { return to_string_range(s.fields_); },
            [](std::monostate) { return string_t{"void"}; }
        },
        data_);
}

const value::array_t& value::to_array() const {
    return visit(
        overloaded{
            [](const std::shared_ptr<array_t>& arr) -> const array_t& { return *arr; },
            [](const auto&) -> const array_t& { throw core::value_error{error_code::invalid_conversion}; }
        },
        data_);
}

value::array_t& value::to_array() {
    return visit(
        overloaded{
            [](std::shared_ptr<array_t>& arr) -> array_t& { return *arr; },
            [](auto&) -> array_t& { throw core::value_error{error_code::invalid_conversion}; }
        },
        data_);
}

const value::struct_t& value::to_struct() const {
    return visit(
        overloaded{
            [](const struct_t& st) -> const struct_t& { return st; },
            [](const auto&) -> const struct_t& { throw core::value_error{error_code::invalid_conversion}; }
        },
        data_);
}

value::struct_t& value::to_struct() {
    return visit(
        overloaded{
            [](struct_t& st) -> struct_t& { return st; },
            [](auto&) -> struct_t& { throw core::value_error{error_code::invalid_conversion}; }
        },
        data_);
}

bool value::is_int() const noexcept { return std::holds_alternative<int_t>(data_); }
bool value::is_double() const noexcept { return std::holds_alternative<double_t>(data_); }
bool value::is_bool() const noexcept { return std::holds_alternative<bool_t>(data_); }
bool value::is_string() const noexcept { return std::holds_alternative<std::shared_ptr<string_t>>(data_); }
bool value::is_array() const noexcept { return std::holds_alternative<std::shared_ptr<array_t>>(data_); }
bool value::is_struct() const noexcept { return std::holds_alternative<struct_t>(data_); }
bool value::is_void() const noexcept { return std::holds_alternative<std::monostate>(data_); }
// clang-format on

}  // namespace core
