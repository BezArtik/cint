// core/value.hpp


#pragma once
#include "core/token/token_types.hpp"
#include <string>
#include <variant>
#include <optional>
#include <cstdint>

namespace core {

class value {
public:	
    using int_t = int64_t;
	using double_t = double;
	using bool_t = bool;
	using string_t = std::string;
	using array_t = std::vector<value>;

    value();

    template <typename T>
    value(T v) : data_(std::move(v)) {}

    core::type type() const;

    int_t to_int() const;
    double_t to_double() const;
    string_t to_string() const;

    template <typename T>
    std::optional<T> as() const noexcept {
        if (auto* p = std::get_if<T>(&data_)) return *p;
        return std::nullopt;
    }
    std::optional<int_t> as_int() const noexcept;
    std::optional<double_t> as_double() const noexcept;
    std::optional<bool_t> as_bool() const noexcept;
    std::optional<string_t> as_string() const noexcept;
	std::optional<array_t> as_array() const noexcept;
	array_t* as_array_mut() noexcept;
    std::optional<array_t> take_array() noexcept;

    size_t array_size() const;

    value add(const value& other) const;
    value sub(const value& other) const;
    value mul(const value& other) const;
    value div(const value& other) const;
    value mod(const value& other) const;

    value eq(const value& other) const;
    value neq(const value& other) const;
    value lt(const value& other) const;
    value le(const value& other) const;
    value gt(const value& other) const;
    value ge(const value& other) const;

    value and_op(const value& other) const;
    value or_op(const value& other) const;
    value not_op() const;

private:
    std::variant<
        int_t,
        double_t,
        bool_t,
        string_t,
		array_t,
        std::monostate
    > data_;
};

} // namespace core