#include "core/value/value.hpp"

#include "core/error/error_codes.hpp"
#include "core/utils/overloaded.hpp"
#include "core/value/conversions.hpp"

#include <cmath>
#include <format>
#include <iterator>
#include <limits>
#include <ranges>
#include <string_view>
#include <variant>

namespace cint::core {

namespace {

[[noreturn]] void invalid_conversion() { throw value_error{error_code::invalid_conversion}; }

[[noreturn]] void division_by_zero() { throw value_error{error_code::division_by_zero}; }

template <typename T>
auto from_string(std::string_view text) {
  auto num = to_numbers<T>(text);
  if (!num) invalid_conversion();
  return *num;
}

}  // namespace

value::int_t value::to_int() const {
  if (auto p = std::get_if<value::int_t>(&data_)) return *p;
  return std::visit(overloaded{[](value::double_t v) {
                                 if (!std::isfinite(v) ||
                                     v < static_cast<value::double_t>(std::numeric_limits<value::int_t>::min()) ||
                                     v > static_cast<value::double_t>(std::numeric_limits<value::int_t>::max()))
                                   invalid_conversion();
                                 return static_cast<value::int_t>(v);
                               },
                               [](value::bool_t v) { return static_cast<value::int_t>(v); },
                               [](const value::string_t& s) { return from_string<value::int_t>(s); },
                               [](const auto&) -> value::int_t { invalid_conversion(); }},
                    data_);
}

value::double_t value::to_double() const {
  if (auto p = std::get_if<value::double_t>(&data_)) return *p;
  return std::visit(overloaded{[](value::int_t v) { return static_cast<value::double_t>(v); },
                               [](value::bool_t v) { return static_cast<value::double_t>(v); },
                               [](const value::string_t& s) { return from_string<value::double_t>(s); },
                               [](const auto&) -> value::double_t { invalid_conversion(); }},
                    data_);
}

value::bool_t value::to_bool() const {
  if (auto p = std::get_if<value::bool_t>(&data_)) return *p;
  return std::visit(overloaded{[](value::int_t v) { return v != 0; }, [](value::double_t v) { return v != 0.0; },
                               [](value::bool_t v) { return v; }, [](const value::string_t& s) { return !s.empty(); },
                               [](const value::struct_impl&) { return true; }, [](value::void_t) { return false; }},
                    data_);
}

value::string_t value::to_string() const {
  auto format_struct = [&](const value::struct_t& s) {
    auto res = value::string_t{};
    res.reserve(s.fields_.size() + s.node_->field_names_.size());
    const auto it = std::back_inserter(res);
    std::format_to(it, "{} (", s.node_->name_);
    for (auto&& [i, name, value] :
         std::views::zip(std::views::iota(0ul, s.fields_.size()), s.node_->field_names_, s.fields_)) {
      if (i) std::format_to(it, ", ");
      std::format_to(it, "{}: {}", name, value.to_string());
    }
    std::format_to(it, ")");
    return res;
  };
  if (auto p = std::get_if<value::string_t>(&data_)) return *p;
  return std::visit(
      overloaded{[](value::int_t v) { return std::format("{}", v); },
                 [](value::double_t v) { return std::format("{}", v); },
                 [](value::bool_t v) { return std::format("{}", v); }, [](const value::string_t& s) { return s; },
                 [&](const value::struct_impl& s) { return format_struct(*s); },
                 [](value::void_t) { return std::string{"void"}; }},
      data_);
}

const value::struct_t& value::to_struct() const {
  if (auto p = std::get_if<value::struct_impl>(&data_)) return **p;
  invalid_conversion();
}

value::struct_t& value::to_struct() {
  if (auto p = std::get_if<value::struct_impl>(&data_)) return **p;
  invalid_conversion();
}

bool value::is_int() const noexcept { return std::holds_alternative<value::int_t>(data_); }
bool value::is_double() const noexcept { return std::holds_alternative<value::double_t>(data_); }
bool value::is_bool() const noexcept { return std::holds_alternative<value::bool_t>(data_); }
bool value::is_string() const noexcept { return std::holds_alternative<value::string_t>(data_); }
bool value::is_struct() const noexcept { return std::holds_alternative<value::struct_impl>(data_); }
bool value::is_void() const noexcept { return std::holds_alternative<value::void_t>(data_); }

void value::make_unique_struct() {
  auto p = std::get_if<value::struct_impl>(&data_);
  if (p && p->use_count() > 1) *p = std::make_shared<value::struct_t>(**p);
}

bool operator==(const value& a, const value& b) noexcept {
  return std::visit(
      overloaded{[](value::int_t l, value::int_t r) { return l == r; },
                 [](value::double_t l, value::double_t r) { return l == r; },
                 [](value::int_t l, value::double_t r) { return static_cast<value::double_t>(l) == r; },
                 [](value::double_t l, value::int_t r) { return l == static_cast<value::double_t>(r); },
                 [](value::bool_t l, value::bool_t r) { return l == r; },
                 [](const value::string_t& l, const value::string_t& r) { return l == r; },
                 [](const value::struct_impl& l, const value::struct_impl& r) {
                   if (l->node_ != r->node_) return false;
                   return l->fields_ == r->fields_;
                 },
                 [](value::void_t, value::void_t) { return true; }, [](const auto&, const auto&) { return false; }},
      a.data_, b.data_);
}

bool operator!=(const value& a, const value& b) noexcept { return !(a == b); }

bool operator<(const value& a, const value& b) {
  return std::visit(overloaded{[](value::int_t l, value::int_t r) { return l < r; },
                               [](value::double_t l, value::double_t r) { return l < r; },
                               [](value::int_t l, value::double_t r) { return static_cast<value::double_t>(l) < r; },
                               [](value::double_t l, value::int_t r) { return l < static_cast<value::double_t>(r); },
                               [](const value::string_t& l, const value::string_t& r) { return l < r; },
                               [](const auto&, const auto&) -> bool { invalid_conversion(); }},
                    a.data_, b.data_);
}

bool operator<=(const value& a, const value& b) {
  return std::visit(overloaded{[](value::int_t l, value::int_t r) { return l <= r; },
                               [](value::double_t l, value::double_t r) { return l <= r; },
                               [](value::int_t l, value::double_t r) { return static_cast<value::double_t>(l) <= r; },
                               [](value::double_t l, value::int_t r) { return l <= static_cast<value::double_t>(r); },
                               [](const value::string_t& l, const value::string_t& r) { return l <= r; },
                               [](const auto&, const auto&) -> bool { invalid_conversion(); }},
                    a.data_, b.data_);
}

bool operator>(const value& a, const value& b) {
  return std::visit(overloaded{[](value::int_t l, value::int_t r) { return l > r; },
                               [](value::double_t l, value::double_t r) { return l > r; },
                               [](value::int_t l, value::double_t r) { return static_cast<value::double_t>(l) > r; },
                               [](value::double_t l, value::int_t r) { return l > static_cast<value::double_t>(r); },
                               [](const value::string_t& l, const value::string_t& r) { return l > r; },
                               [](const auto&, const auto&) -> bool { invalid_conversion(); }},
                    a.data_, b.data_);
}

bool operator>=(const value& a, const value& b) {
  return std::visit(overloaded{[](value::int_t l, value::int_t r) { return l >= r; },
                               [](value::double_t l, value::double_t r) { return l >= r; },
                               [](value::int_t l, value::double_t r) { return static_cast<value::double_t>(l) >= r; },
                               [](value::double_t l, value::int_t r) { return l >= static_cast<value::double_t>(r); },
                               [](const value::string_t& l, const value::string_t& r) { return l >= r; },
                               [](const auto&, const auto&) -> bool { invalid_conversion(); }},
                    a.data_, b.data_);
}

value operator+(const value& a, const value& b) {
  return std::visit(
      overloaded{[](value::int_t l, value::int_t r) -> value { return l + r; },
                 [](value::int_t l, value::double_t r) -> value { return static_cast<value::double_t>(l) + r; },
                 [](value::double_t l, value::int_t r) -> value { return l + static_cast<value::double_t>(r); },
                 [](value::double_t l, value::double_t r) -> value { return l + r; },
                 [](const value::string_t& l, const value::string_t& r) -> value { return l + r; },
                 [](const auto&, const auto&) -> value { invalid_conversion(); }},
      a.data_, b.data_);
}

value operator-(const value& a, const value& b) {
  return std::visit(
      overloaded{[](value::int_t l, value::int_t r) -> value { return l - r; },
                 [](value::int_t l, value::double_t r) -> value { return static_cast<value::double_t>(l) - r; },
                 [](value::double_t l, value::int_t r) -> value { return l - static_cast<value::double_t>(r); },
                 [](value::double_t l, value::double_t r) -> value { return l - r; },
                 [](const auto&, const auto&) -> value { invalid_conversion(); }},
      a.data_, b.data_);
}

value operator*(const value& a, const value& b) {
  return std::visit(
      overloaded{[](value::int_t l, value::int_t r) -> value { return l * r; },
                 [](value::int_t l, value::double_t r) -> value { return static_cast<value::double_t>(l) * r; },
                 [](value::double_t l, value::int_t r) -> value { return l * static_cast<value::double_t>(r); },
                 [](value::double_t l, value::double_t r) -> value { return l * r; },
                 [](const value::string_t& s, value::int_t n) -> value {
                   if (n < 0) invalid_conversion();
                   std::string out;
                   out.reserve(s.size() * static_cast<std::size_t>(n));
                   for (const auto _ : std::views::iota(0, n)) out += s;
                   return out;
                 },
                 [](value::int_t n, const value::string_t& s) -> value { return s * n; },
                 [](const auto&, const auto&) -> value { invalid_conversion(); }},
      a.data_, b.data_);
}

value operator/(const value& a, const value& b) {
  return std::visit(overloaded{[](value::int_t l, value::int_t r) -> value {
                                 if (r == 0) division_by_zero();
                                 return l / r;
                               },
                               [](value::int_t l, value::double_t r) -> value {
                                 if (r == 0.0) division_by_zero();
                                 return static_cast<value::double_t>(l) / r;
                               },
                               [](value::double_t l, value::int_t r) -> value {
                                 if (r == 0) division_by_zero();
                                 return l / static_cast<value::double_t>(r);
                               },
                               [](value::double_t l, value::double_t r) -> value {
                                 if (r == 0.0) division_by_zero();
                                 return l / r;
                               },
                               [](const auto&, const auto&) -> value { invalid_conversion(); }},
                    a.data_, b.data_);
}

value operator%(const value& a, const value& b) {
  return std::visit(overloaded{[](value::int_t l, value::int_t r) -> value {
                                 if (r == 0) division_by_zero();
                                 return l % r;
                               },
                               [](const auto&, const auto&) -> value { invalid_conversion(); }},
                    a.data_, b.data_);
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

bool operator!(const value& a) noexcept { return !a.to_bool(); }

value operator&(const value& a, const value& b) {
  return std::visit(overloaded{[](value::int_t l, value::int_t r) -> value { return l & r; },
                               [](const auto&, const auto&) -> value { invalid_conversion(); }},
                    a.data_, b.data_);
}

value operator|(const value& a, const value& b) {
  return std::visit(overloaded{[](value::int_t l, value::int_t r) -> value { return l | r; },
                               [](const auto&, const auto&) -> value { invalid_conversion(); }},
                    a.data_, b.data_);
}

value operator^(const value& a, const value& b) {
  return std::visit(overloaded{[](value::int_t l, value::int_t r) -> value { return l ^ r; },
                               [](const auto&, const auto&) -> value { invalid_conversion(); }},
                    a.data_, b.data_);
}

value operator<<(const value& a, const value& b) {
  return std::visit(overloaded{[](value::int_t l, value::int_t r) -> value { return l << r; },
                               [](const auto&, const auto&) -> value { invalid_conversion(); }},
                    a.data_, b.data_);
}

value operator>>(const value& a, const value& b) {
  return std::visit(overloaded{[](value::int_t l, value::int_t r) -> value { return l >> r; },
                               [](const auto&, const auto&) -> value { invalid_conversion(); }},
                    a.data_, b.data_);
}

}  // namespace cint::core
