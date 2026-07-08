// core/value/value.hpp

#pragma once
#include "core/token/token_types.hpp"
#include "core/utils/shared_data.hpp"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace core {

class value {
public:
    using int_t = int64_t;
    using double_t = double;
    using bool_t = bool;
    using string_t = shared_data<std::string>;
    using array_t = shared_data<std::vector<value>>;

    value();
    value(std::monostate) : data_(std::monostate{}) {}

    template <typename T>
    value(T v) : data_(std::move(v)) {}

    core::type type() const;

    int_t to_int() const;
    double_t to_double() const;
    bool_t to_bool() const;
    std::string to_string() const;

    bool is_int() const noexcept;
    bool is_double() const noexcept;
    bool is_bool() const noexcept;
    bool is_string() const noexcept;
    bool is_array() const noexcept;
    bool is_void() const noexcept;

    template <typename T>
    const T* as() const noexcept {
        return std::get_if<T>(&data_);
    }

    template <typename T>
    T* as_mut() noexcept {
        return std::get_if<T>(&data_);
    }

private:
    std::variant<int_t, double_t, bool_t, string_t, array_t, std::monostate> data_;
};

}  // namespace core
