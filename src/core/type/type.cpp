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

type type::int_type() { return type(int_t{}); }
type type::double_type() { return type(double_t{}); }
type type::bool_type() { return type(bool_t{}); }
type type::string_type() { return type(string_t{}); }
type type::void_type() { return type(void_t{}); }
type type::unknown_type() { return type(unknown_t{}); }
type type::function_type(std::string_view name, type return_type, std::vector<param_t> params) {
    return std::make_shared<function_t>(name, std::make_unique<type>(std::move(return_type)), std::move(params));
}
type type::array_type(type element_type, size_t size) {
    return std::make_shared<array_t>(std::make_unique<type>(std::move(element_type)), size);
}
type type::struct_type(std::string_view name, std::vector<field_t> fields) {
    return std::make_shared<struct_t>(name, std::move(fields));
}

bool type::is_int()      const noexcept { return std::holds_alternative<int_t>(data_); }
bool type::is_double()   const noexcept { return std::holds_alternative<double_t>(data_); }
bool type::is_bool()     const noexcept { return std::holds_alternative<bool_t>(data_); }
bool type::is_string()   const noexcept { return std::holds_alternative<string_t>(data_); }
bool type::is_void()     const noexcept { return std::holds_alternative<void_t>(data_); }
bool type::is_unknown()  const noexcept { return std::holds_alternative<unknown_t>(data_); }
bool type::is_function() const noexcept { return std::holds_alternative<std::shared_ptr<function_t>>(data_); }
bool type::is_array()    const noexcept { return std::holds_alternative<std::shared_ptr<array_t>>(data_); }
bool type::is_struct()   const noexcept { return std::holds_alternative<std::shared_ptr<struct_t>>(data_); }
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

std::string_view type::function_name() const { return std::get<std::shared_ptr<function_t>>(data_)->name_; }
const type& type::return_type() const { return *std::get<std::shared_ptr<function_t>>(data_)->return_type_; }
std::span<const type::param_t> type::param_infos() const { return std::get<std::shared_ptr<function_t>>(data_)->params_; }

const type& type::element_type() const { return *std::get<std::shared_ptr<array_t>>(data_)->element_type_; }
size_t type::array_size() const { return std::get<std::shared_ptr<array_t>>(data_)->size_; }

std::string_view type::struct_name() const { return std::get<std::shared_ptr<struct_t>>(data_)->name_; }
std::span<const type::field_t> type::struct_fields() const { return std::get<std::shared_ptr<struct_t>>(data_)->fields_; }

std::optional<size_t> type::field_index(std::string_view name) const noexcept {
    auto&& fields = struct_fields();
    auto&& it = std::ranges::find(fields, name, &field_t::first);
    if (it == fields.end()) return std::nullopt;
    return std::distance(fields.begin(), it);
}
// clang-format on

}  // namespace core
