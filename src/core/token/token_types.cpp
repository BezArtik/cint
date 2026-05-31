// core/token/token_types.cpp

#include "core/token/token_types.hpp"
#include <vector>
#include <memory>
#include <variant>
#include <utility>
#include <algorithm>
#include <ranges>

namespace core {

type::type(kind k) : kind_(k), info_(std::monostate{}) {}
type::type(kind k, function_info info) : kind_(k), info_(std::move(info)) {}

type::type(const type& other) : kind_(other.kind_) {
    if (kind_ == kind::FUNCTION) {
        const auto& src = std::get<function_info>(other.info_);
        info_ = function_info{
            std::make_unique<type>(*src.return_type),
            src.param_types
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
	info.return_type = std::make_unique<type>(std::move(return_type));
	info.param_types = std::move(param_types);
	return type(kind::FUNCTION, std::move(info));
}

bool type::is_primitive() const noexcept {
    return kind_ == kind::INT || kind_ == kind::DOUBLE ||
           kind_ == kind::BOOL || kind_ == kind::STRING;
}

bool type::is_numeric() const noexcept {
    return kind_ == kind::INT || kind_ == kind::DOUBLE;
}

bool type::is_void() const noexcept { return kind_ == kind::VOID; }
bool type::is_function() const noexcept { return kind_ == kind::FUNCTION; }
bool type::is_unknown() const noexcept { return kind_ == kind::UNKNOWN; }

const type& type::return_type() const {
    return *std::get<function_info>(info_).return_type;
}

const std::vector<type>& type::param_types() const {
    return std::get<function_info>(info_).param_types;
}

bool type::operator==(const type& other) const noexcept {
    if (kind_ != other.kind_) return false;

    if (kind_ == kind::FUNCTION) {
        const auto& lhs_info = std::get<function_info>(info_);
        const auto& rhs_info = std::get<function_info>(other.info_);

        if (*lhs_info.return_type != *rhs_info.return_type) return false;
        return lhs_info.param_types.size() == rhs_info.param_types.size() && 
            std::ranges::equal(lhs_info.param_types, rhs_info.param_types);
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

type type::common_arithmetic_type(const type& other) const noexcept {
    if (!is_numeric() || !other.is_numeric()) return unknown_type();
    if (kind_ == kind::DOUBLE || other.kind_ == kind::DOUBLE) return double_type();
    return int_type();
}


}
