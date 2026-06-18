// core/token/token_types.cpp

#include "core/token/token_types.hpp"
#include <vector>
#include <memory>
#include <variant>
#include <utility>
#include <algorithm>
#include <cassert>

namespace core {

type::type(kind k) : kind_(k), info_(std::monostate{}) {}

type::type(const type& other) : kind_(other.kind_) {
    if (kind_ == kind::FUNCTION) {
        const auto& src = std::get<function_info>(other.info_);
        info_ = function_info{
            std::make_unique<type>(*src.return_type_),
            src.param_types_
        };
    } else if (kind_ == kind::ARRAY) {
        const auto& src = std::get<array_info>(other.info_);
        info_ = array_info{
            std::make_unique<type>(*src.element_type_),
            src.size_
        };
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

type type::int_type() { return type(kind::INT); }
type type::double_type() { return type(kind::DOUBLE); }
type type::bool_type() { return type(kind::BOOL); }
type type::string_type() { return type(kind::STRING); }
type type::void_type() { return type(kind::VOID); }
type type::unknown_type() { return type(kind::UNKNOWN); }

type type::function_type(type return_type, std::vector<type> param_types) {
    function_info info;
    info.return_type_ = std::make_unique<type>(std::move(return_type));
    info.param_types_ = std::move(param_types);
    return type(kind::FUNCTION, std::move(info));
}

type type::array_type(type element_type, size_t size) {
	array_info info;
	info.element_type_ = std::make_unique<type>(std::move(element_type));
	info.size_ = size;
	return type(kind::ARRAY, std::move(info));
}

bool type::is_primitive() const noexcept {
    return kind_ == kind::INT || kind_ == kind::DOUBLE ||
        kind_ == kind::BOOL || kind_ == kind::STRING;
}

bool type::is_numeric() const noexcept {
    return kind_ == kind::INT || kind_ == kind::DOUBLE;
}

bool type::is_int() const noexcept { return kind_ == kind::INT; }
bool type::is_double() const noexcept { return kind_ == kind::DOUBLE; }
bool type::is_bool() const noexcept { return kind_ == kind::BOOL; }
bool type::is_string() const noexcept { return kind_ == kind::STRING; }
bool type::is_void() const noexcept { return kind_ == kind::VOID; }
bool type::is_function() const noexcept { return kind_ == kind::FUNCTION; }
bool type::is_unknown() const noexcept { return kind_ == kind::UNKNOWN; }
bool type::is_array() const noexcept { return kind_ == kind::ARRAY; }

const type& type::return_type() const {
	assert(is_function());
    return *std::get<function_info>(info_).return_type_;
}

const std::vector<type>& type::param_types() const {
	assert(is_function());
    return std::get<function_info>(info_).param_types_;
}

bool type::operator==(const type& other) const noexcept {
    if (kind_ != other.kind_) return false;

    if (kind_ == kind::FUNCTION) {
        const auto& lhs_info = std::get<function_info>(info_);
        const auto& rhs_info = std::get<function_info>(other.info_);

        if (*lhs_info.return_type_ != *rhs_info.return_type_) return false;
        return lhs_info.param_types_.size() == rhs_info.param_types_.size() &&
            std::ranges::equal(lhs_info.param_types_, rhs_info.param_types_);
    } else if (kind_ == kind::ARRAY) {
        const auto& lhs = std::get<array_info>(info_);
        const auto& rhs = std::get<array_info>(other.info_);
        if (*lhs.element_type_ != *rhs.element_type_) return false;
        return lhs.size_ == rhs.size_ || lhs.size_ == 0 || rhs.size_ == 0;
    }

    return true;
}

bool type::operator!=(const type& other) const noexcept {
    return !(*this == other);
}

bool type::is_assignable_from(const type& source) const noexcept {
    if (*this == source) return true;

    if (kind_ == kind::DOUBLE && source.kind_ == kind::INT) return true;
    if (kind_ == kind::ARRAY && source.kind_ == kind::ARRAY) {
        return element_type().is_assignable_from(source.element_type())
            && (array_size() == 0 || source.array_size() == 0
                || array_size() == source.array_size());
    }
    return false;
}

type type::common_arithmetic_type(const type& other) const noexcept {
    if (!is_numeric() || !other.is_numeric()) return unknown_type();
    if (kind_ == kind::DOUBLE || other.kind_ == kind::DOUBLE) return double_type();
    return int_type();
}

const type& type::element_type() const {
	assert(is_array());
    return *std::get<array_info>(info_).element_type_;
}

size_t type::array_size() const {
	assert(is_array());
    return std::get<array_info>(info_).size_;
}

}