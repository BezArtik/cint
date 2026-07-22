// core/token/type.cpp

#include "core/token/type.hpp"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <variant>
#include <vector>

namespace core {

type::type(const type& other) : kind_(other.kind_) {
    if (is_function()) {
        const auto& src = std::get<function_info>(other.info_);
        info_ = function_info{std::make_unique<type>(*src.return_type_), src.param_types_};
    } else if (is_array()) {
        const auto& src = std::get<array_info>(other.info_);
        info_ = array_info{std::make_unique<type>(*src.element_type_), src.size_};
    } else if (is_struct()) {
        info_ = std::get<struct_info>(other.info_);
    }
}

type& type::operator=(const type& other) {
    type tmp(other);
    swap(tmp);
    return *this;
}

void type::swap(type& other) noexcept {
    std::swap(kind_, other.kind_);
    info_.swap(other.info_);
}

type type::function_type(type return_type, std::vector<type> param_types) {
    function_info info{std::make_unique<type>(std::move(return_type)), std::move(param_types)};
    return type{kind::FUNCTION, std::move(info)};
}

type type::array_type(type element_type, size_t size) {
    array_info info{std::make_unique<type>(std::move(element_type)), size};
    return type{kind::ARRAY, std::move(info)};
}

type type::struct_type(std::string_view name, std::vector<field_t> fields) {
    struct_info info{name, std::move(fields)};
    return type{kind::STRUCT, std::move(info)};
}

bool type::is_primitive() const noexcept {
    return kind_ == kind::INT || kind_ == kind::DOUBLE || kind_ == kind::BOOL || kind_ == kind::STRING;
}

bool type::is_numeric() const noexcept {
    return kind_ == kind::INT || kind_ == kind::DOUBLE;
}
// clang-format off
bool type::is_int() const noexcept { return kind_ == kind::INT; }
bool type::is_double() const noexcept { return kind_ == kind::DOUBLE; }
bool type::is_bool() const noexcept { return kind_ == kind::BOOL; }
bool type::is_string() const noexcept { return kind_ == kind::STRING; }
bool type::is_void() const noexcept { return kind_ == kind::VOID; }
bool type::is_function() const noexcept { return kind_ == kind::FUNCTION; }
bool type::is_unknown() const noexcept { return kind_ == kind::UNKNOWN; }
bool type::is_array() const noexcept { return kind_ == kind::ARRAY; }
bool type::is_struct() const noexcept { return kind_ == kind::STRUCT; }
// clang-format on

bool type::operator==(const type& other) const noexcept {
    if (kind_ != other.kind_) return false;

    if (is_function()) {
        const auto& lhs_info = std::get<function_info>(info_);
        const auto& rhs_info = std::get<function_info>(other.info_);

        if (*lhs_info.return_type_ != *rhs_info.return_type_) return false;
        return lhs_info.param_types_.size() == rhs_info.param_types_.size() &&
               std::ranges::equal(lhs_info.param_types_, rhs_info.param_types_);
    } else if (is_array()) {
        return false;
    } else if (is_struct()) {
        const auto& lhs_info = std::get<struct_info>(info_);
        const auto& rhs_info = std::get<struct_info>(other.info_);
        return lhs_info.name_ == rhs_info.name_;
    }

    return true;
}

bool type::operator!=(const type& other) const noexcept {
    return !(*this == other);
}

bool type::is_assignable_from(const type& source) const noexcept {
    if (*this == source) return true;
    if (kind_ == kind::DOUBLE && source.kind_ == kind::INT) return true;
    return false;
}

const type& type::return_type() const {
    assert(is_function());
    return *std::get<function_info>(info_).return_type_;
}

std::span<const type> type::param_types() const {
    assert(is_function());
    return std::get<function_info>(info_).param_types_;
}

const type& type::element_type() const {
    assert(is_array());
    return *std::get<array_info>(info_).element_type_;
}

size_t type::array_size() const {
    assert(is_array());
    return std::get<array_info>(info_).size_;
}

std::string_view type::struct_name() const {
    assert(is_struct());
    return std::get<struct_info>(info_).name_;
}

std::span<const type::field_t> type::struct_fields() const {
    assert(is_struct());
    return std::get<struct_info>(info_).fields_;
}

std::optional<size_t> type::field_index(std::string_view name) const noexcept {
    assert(is_struct());
    const auto& fields = std::get<struct_info>(info_).fields_;
    auto it = std::ranges::find(fields, name, &field_t::first);
    if (it == fields.end()) return std::nullopt;
    return std::distance(fields.begin(), it);
}

type::kind type::get_kind() const noexcept {
    return kind_;
}

}  // namespace core
