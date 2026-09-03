// core/type/type.cpp

#include "core/type/type.hpp"

#include <algorithm>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace core {
// clang-format off

struct type::function_t {
    std::string_view name_;
    type return_type_;
    std::vector<param_t> params_;
};

struct type::array_t {
    type element_type_;
    size_t size_;
};

struct type::struct_t {
    std::string_view name_;
    std::vector<field_t> fields_;
};

type type::int_type() { return int_t{}; }
type type::double_type() { return double_t{}; }
type type::bool_type() { return bool_t{}; }
type type::string_type() { return string_t{}; }
type type::void_type() { return void_t{}; }
type type::unknown_type() { return unknown_t{}; }
type type::function_type(std::string_view name, type return_type, std::vector<param_t> params) {
    return std::make_shared<function_t>(name, std::move(return_type), std::move(params));
}
type type::array_type(type element_type, size_t size) {
    return std::make_shared<array_t>(std::move(element_type), size);
}
type type::struct_type(std::string_view name, std::vector<field_t> fields) {
    return std::make_shared<struct_t>(name, std::move(fields));
}

bool type::is_int()      const noexcept { return data_.holds<int_t>(); }
bool type::is_double()   const noexcept { return data_.holds<double_t>(); }
bool type::is_bool()     const noexcept { return data_.holds<bool_t>(); }
bool type::is_string()   const noexcept { return data_.holds<string_t>(); }
bool type::is_void()     const noexcept { return data_.holds<void_t>(); }
bool type::is_unknown()  const noexcept { return data_.holds<unknown_t>(); }
bool type::is_function() const noexcept { return data_.holds<func_wrap>(); }
bool type::is_array()    const noexcept { return data_.holds<array_wrap>(); }
bool type::is_struct()   const noexcept { return data_.holds<struct_wrap>(); }
bool type::is_numeric()  const noexcept { return is_int() || is_double(); }

bool type::operator==(const type& other) const noexcept {
    if (data_.index() != other.data_.index()) return false;

    if (is_function()) {
        if (return_type() != other.return_type()) return false;
        return param_infos().size() == other.param_infos().size() &&
               std::ranges::equal(param_infos(), other.param_infos());
    }
    if (is_array()) return array_size() == other.array_size() && element_type() == other.element_type();
    if (is_struct()) return struct_name() == other.struct_name();
    
    return true;
}

bool type::operator!=(const type& other) const noexcept { return !(*this == other); }

std::string_view type::function_name() const { return data_.get<func_wrap>()->name_; }
const type& type::return_type() const { return data_.get<func_wrap>()->return_type_; }
std::span<const type::param_t> type::param_infos() const { return data_.get<func_wrap>()->params_; }

const type& type::element_type() const { return data_.get<array_wrap>()->element_type_; }
size_t type::array_size() const { return data_.get<array_wrap>()->size_; }

std::string_view type::struct_name() const { return data_.get<struct_wrap>()->name_; }
std::span<const type::field_t> type::struct_fields() const { return data_.get<struct_wrap>()->fields_; }

std::optional<size_t> type::field_index(std::string_view name) const noexcept {
    auto&& fields = struct_fields();
    auto&& it = std::ranges::find(fields, name, &field_t::first);
    if (it == fields.end()) return std::nullopt;
    return std::distance(fields.begin(), it);
}
// clang-format on

}  // namespace core
