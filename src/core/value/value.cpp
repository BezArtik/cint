#include "core/value/value.hpp"

#include "core/utils/overloaded.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <format>
#include <limits>
#include <print>
#include <ranges>
#include <sstream>

namespace cint::core {

namespace {

[[noreturn]] void invalid_conversion() {
    throw value_error{error_code::invalid_conversion};
}

value::int_t parse_int(std::string_view text) {
    auto result = value::int_t{};
    auto [ptr, ec] = std::from_chars(text.begin(), text.end(), result);
    if (ec != std::errc{} || ptr != text.end()) invalid_conversion();
    return result;
}

value::double_t parse_double(std::string_view text) {
    auto result = value::double_t{};
    auto [ptr, ec] = std::from_chars(text.begin(), text.end(), result);
    if (ec != std::errc{} || ptr != text.end()) invalid_conversion();
    return result;
}

bool is_numeric(const value& v) noexcept {
    return v.is_int() || v.is_double();
}

value::double_t as_double(const value& v) {
    if (v.is_int()) return static_cast<value::double_t>(v.to_int());
    if (v.is_double()) return v.to_double();
    invalid_conversion();
}

std::string format_struct(const value::struct_t& s) {
    std::ostringstream os;
    std::print(os, "{}", s.type_->name_);
    std::print(os, "(");
    for (std::size_t i = 0; i < s.fields_.size(); ++i) {
        if (i) std::print(os, ", ");
        std::print(os, "{}: {}", s.type_->field_names_[i], s.fields_[i].to_string());
    }
    std::print(os, ")");
    return os.str();
}

}  // namespace

value value::from_string(std::string_view text, bool is_double) {
    if (is_double) return parse_double(text);
    return parse_int(text);
}

// clang-format off
value::int_t value::to_int() const {
    if (auto p = data_.get_if<int_t>()) return *p;
    return data_.visit(
            overloaded{
            [](double_t v) -> int_t {
                if (!std::isfinite(v) || v < static_cast<double_t>(std::numeric_limits<int_t>::min()) ||
                                         v > static_cast<double_t>(std::numeric_limits<int_t>::max()))
                    invalid_conversion();
                return static_cast<int_t>(v);
            },
            [](bool_t v) { return static_cast<int_t>(v); }, 
            [](const string_t& s) { return parse_int(s); },
            [](const auto&) -> int_t { invalid_conversion(); }});
}

value::double_t value::to_double() const {
    if (auto p = data_.get_if<double_t>()) return *p;
    return data_.visit(
            overloaded{
            [](int_t v) { return static_cast<double_t>(v); }, 
            [](bool_t v) { return static_cast<double_t>(v); },
            [](const string_t& s) { return parse_double(s); },
            [](const auto&) -> double_t { invalid_conversion(); }});
}

value::bool_t value::to_bool() const {
    if (auto p = data_.get_if<bool_t>()) return *p;
    return data_.visit(
            overloaded{
            [](int_t v) { return v != 0; }, 
            [](double_t v) { return v != 0.0; },
            [](bool_t v) { return v; }, 
            [](const string_t& s) { return !s.empty(); },
            [](const struct_impl&) { return true; }, 
            [](std::monostate) { return false; }});
}

value::string_t value::to_string() const {
    if (auto p = data_.get_if<string_t>()) return *p;
    return data_.visit(
            overloaded{
            [](int_t v) { return std::format("{}", v); }, 
            [](double_t v) { return std::format("{}", v); },
            [](bool_t v) { return std::format("{}", v); }, 
            [](const string_t& s) { return s; },
            [](const struct_impl& s) { return format_struct(*s); }, 
            [](std::monostate) { return std::string{"void"}; }});
}

const value::struct_t& value::to_struct() const {
    if (auto p = data_.get_if<struct_impl>()) return **p;
    invalid_conversion();
}

value::struct_t& value::to_struct() {
    if (auto p = data_.get_if<struct_impl>()) return **p;
    invalid_conversion();
}
// clang-format on

bool value::is_int() const noexcept {
    return data_.holds<int_t>();
}
bool value::is_double() const noexcept {
    return data_.holds<double_t>();
}
bool value::is_bool() const noexcept {
    return data_.holds<bool_t>();
}
bool value::is_string() const noexcept {
    return data_.holds<string_t>();
}
bool value::is_struct() const noexcept {
    return data_.holds<struct_impl>();
}
bool value::is_void() const noexcept {
    return data_.holds<std::monostate>();
}

void value::make_unique_struct() {
    auto p = data_.get_if<struct_impl>();
    if (p && p->use_count() > 1) *p = std::make_shared<struct_t>(**p);
}

bool operator==(const value& a, const value& b) noexcept {
    if (is_numeric(a) && is_numeric(b)) return as_double(a) == as_double(b);
    if (a.is_bool() && b.is_bool()) return a.to_bool() == b.to_bool();

    if (a.is_string() && b.is_string()) return a.to_string() == b.to_string();

    if (a.is_struct() && b.is_struct()) {
        auto& sa = a.to_struct();
        auto& sb = b.to_struct();
        if (sa.type_ != sb.type_) return false;
        return std::ranges::equal(sa.fields_, sb.fields_);
    }

    if (a.is_void() && b.is_void()) return true;

    return false;
}

bool operator!=(const value& a, const value& b) noexcept {
    return !(a == b);
}

bool operator<(const value& a, const value& b) {
    if (is_numeric(a) && is_numeric(b)) return as_double(a) < as_double(b);
    if (a.is_string() && b.is_string()) return a.to_string() < b.to_string();
    invalid_conversion();
}
bool operator<=(const value& a, const value& b) {
    if (is_numeric(a) && is_numeric(b)) return as_double(a) <= as_double(b);
    if (a.is_string() && b.is_string()) return a.to_string() <= b.to_string();
    invalid_conversion();
}
bool operator>(const value& a, const value& b) {
    if (is_numeric(a) && is_numeric(b)) return as_double(a) > as_double(b);
    if (a.is_string() && b.is_string()) return a.to_string() > b.to_string();
    invalid_conversion();
}
bool operator>=(const value& a, const value& b) {
    if (is_numeric(a) && is_numeric(b)) return as_double(a) >= as_double(b);
    if (a.is_string() && b.is_string()) return a.to_string() >= b.to_string();
    invalid_conversion();
}

value operator+(const value& a, const value& b) {
    if (a.is_int() && b.is_int()) return a.to_int() + b.to_int();
    if (is_numeric(a) && is_numeric(b)) return as_double(a) + as_double(b);
    if (a.is_string() && b.is_string()) return a.to_string() + b.to_string();
    invalid_conversion();
}

value operator-(const value& a, const value& b) {
    if (a.is_int() && b.is_int()) return a.to_int() - b.to_int();
    if (is_numeric(a) && is_numeric(b)) return as_double(a) - as_double(b);
    invalid_conversion();
}

value operator*(const value& a, const value& b) {
    if (a.is_int() && b.is_int()) return a.to_int() * b.to_int();
    if (is_numeric(a) && is_numeric(b)) return as_double(a) * as_double(b);

    if (a.is_string() && b.is_int()) {
        auto&& n = b.to_int();
        if (n < 0) invalid_conversion();
        auto&& s = a.to_string();
        std::string out;
        out.reserve(s.size() * static_cast<std::size_t>(n));
        for (const auto _ : std::views::iota(0, n)) out += s;
        return out;
    }
    if (a.is_int() && b.is_string()) return b * a;
    invalid_conversion();
}

value operator/(const value& a, const value& b) {
    if (a.is_int() && b.is_int()) {
        if (b.to_int() == 0) throw value_error{error_code::division_by_zero};
        return a.to_int() / b.to_int();
    }
    if (is_numeric(a) && is_numeric(b)) {
        auto d = as_double(b);
        if (d == 0.0) throw value_error{error_code::division_by_zero};
        return as_double(a) / d;
    }
    invalid_conversion();
}

value operator%(const value& a, const value& b) {
    if (a.is_int() && b.is_int()) {
        if (b.to_int() == 0) throw value_error{error_code::modulo_by_zero};
        return a.to_int() % b.to_int();
    }
    invalid_conversion();
}

value operator-(const value& a) {
    if (a.is_int()) return -a.to_int();
    if (a.is_double()) return -a.to_double();
    invalid_conversion();
}

value operator~(const value& a) {
    if (a.is_int()) return ~a.to_int();
    invalid_conversion();
}

bool operator!(const value& a) noexcept {
    return !a.to_bool();
}

value operator&(const value& a, const value& b) {
    if (a.is_int() && b.is_int()) return a.to_int() & b.to_int();
    invalid_conversion();
}
value operator|(const value& a, const value& b) {
    if (a.is_int() && b.is_int()) return a.to_int() | b.to_int();
    invalid_conversion();
}
value operator^(const value& a, const value& b) {
    if (a.is_int() && b.is_int()) return a.to_int() ^ b.to_int();
    invalid_conversion();
}
value operator<<(const value& a, const value& b) {
    if (a.is_int() && b.is_int()) return a.to_int() << b.to_int();
    invalid_conversion();
}
value operator>>(const value& a, const value& b) {
    if (a.is_int() && b.is_int()) return a.to_int() >> b.to_int();
    invalid_conversion();
}

value& value::operator++() {
    if (is_int()) {
        data_.get<int_t>()++;
        return *this;
    }
    if (is_double()) {
        data_.get<double_t>() += 1.0;
        return *this;
    }
    invalid_conversion();
}

value& value::operator--() {
    if (is_int()) {
        data_.get<int_t>()--;
        return *this;
    }
    if (is_double()) {
        data_.get<double_t>() -= 1.0;
        return *this;
    }
    invalid_conversion();
}

value value::operator++(int) {
    auto old = *this;
    ++*this;
    return old;
}

value value::operator--(int) {
    auto old = *this;
    --*this;
    return old;
}

}  // namespace cint::core
