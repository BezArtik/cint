// core/type/struct_type.hpp
#pragma once
#include <optional>
#include <string_view>
#include <vector>

namespace cint::core {

struct struct_type {
    std::string_view name_;
    std::vector<std::string_view> field_names_;

    std::optional<size_t> field_index(std::string_view name) const noexcept {
        const auto it = std::ranges::find(field_names_, name);
        if (it == field_names_.end()) return std::nullopt;
        return std::distance(field_names_.begin(), it);
    }
};

}  // namespace cint::core
