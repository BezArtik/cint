// core/value/value.hpp

#pragma once
#include "core/token/type.hpp"
#include "core/utils/shared_data.hpp"

#include <cstdint>
#include <string>
#include <string_view>
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

    static value default_value(const core::type& t);
    static value convert(core::value val, const core::type& target);
    static value from_string(std::string_view text, bool is_double);

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
    static int_t parse_int(std::string_view text);
    static double_t parse_double(std::string_view text);

    std::variant<int_t, double_t, bool_t, string_t, array_t, std::monostate> data_;
};

}  // namespace core
