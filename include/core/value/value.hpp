// core/value/value.hpp

#pragma once
#include "core/error/error_codes.hpp"
#include "core/type/struct_type.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace cint::core {

struct value_error : std::exception {
    error_code code_;

    value_error(error_code c) noexcept : code_{c} {}
    [[nodiscard]] const char* what() const noexcept override { return "Value error"; }
};

class value {
public:
    using int_t = std::int64_t;
    using double_t = double;
    using bool_t = bool;
    using string_t = std::string;
    struct struct_t {
        const struct_type* type_;
        std::vector<value> fields_;
    };

    value() = default;
    value(int_t v) : data_{std::move(v)} {}
    value(double_t v) : data_{std::move(v)} {}
    value(bool_t v) : data_{std::move(v)} {}
    value(string_t v) : data_{std::move(v)} {}
    value(struct_t v) : data_{std::make_shared<struct_t>(std::move(v))} {}

    [[nodiscard]] int_t to_int() const;
    [[nodiscard]] double_t to_double() const;
    [[nodiscard]] bool_t to_bool() const;
    [[nodiscard]] string_t to_string() const;
    [[nodiscard]] const struct_t& to_struct() const;
    [[nodiscard]] struct_t& to_struct();

    [[nodiscard]] bool is_int() const noexcept;
    [[nodiscard]] bool is_double() const noexcept;
    [[nodiscard]] bool is_bool() const noexcept;
    [[nodiscard]] bool is_string() const noexcept;
    [[nodiscard]] bool is_struct() const noexcept;
    [[nodiscard]] bool is_void() const noexcept;

    void make_unique_struct();

    friend bool operator==(const value& a, const value& b) noexcept;
    friend bool operator!=(const value& a, const value& b) noexcept;
    friend bool operator<(const value& a, const value& b);
    friend bool operator<=(const value& a, const value& b);
    friend bool operator>(const value& a, const value& b);
    friend bool operator>=(const value& a, const value& b);

    friend value operator+(const value& a, const value& b);
    friend value operator-(const value& a, const value& b);
    friend value operator*(const value& a, const value& b);
    friend value operator/(const value& a, const value& b);
    friend value operator%(const value& a, const value& b);

    friend value operator-(const value& a);
    friend value operator~(const value& a);
    friend bool operator!(const value& a) noexcept;

    friend value operator&(const value& a, const value& b);
    friend value operator|(const value& a, const value& b);
    friend value operator^(const value& a, const value& b);
    friend value operator<<(const value& a, const value& b);
    friend value operator>>(const value& a, const value& b);

private:
    using struct_impl = std::shared_ptr<struct_t>;
    using void_t = std::monostate;

    std::variant<void_t, int_t, double_t, bool_t, string_t, struct_impl> data_;
};

}  // namespace cint::core
