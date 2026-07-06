// core/value/value.hpp

#pragma once
#include "core/token/token_types.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <variant>

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
    value(core::type element_type, array_t elements);

    core::type type() const;

    int_t to_int() const;
    double_t to_double() const;
    string_t to_string() const;
    bool_t to_bool() const;

    template <typename T>
    std::optional<T> as() const noexcept {
        if (auto* p = std::get_if<T>(&data_)) return *p;
        return std::nullopt;
    }
    std::optional<int_t> as_int() const noexcept;
    std::optional<double_t> as_double() const noexcept;
    std::optional<bool_t> as_bool() const noexcept;
    std::optional<string_t> as_string() const noexcept;
    const array_t* as_array() const noexcept;
    array_t* as_array() noexcept;

private:
    struct array_info {
        core::type element_type_;
        array_t elements_;
    };
    std::variant<int_t, double_t, bool_t, string_t, array_info, std::monostate> data_;
};

}  // namespace core
