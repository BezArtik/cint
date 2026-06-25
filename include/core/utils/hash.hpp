// core/utils/hash.hpp

#pragma once
#include <functional>
#include <string_view>

namespace core {

struct transparent_string_hash {
    using is_transparent = void;

    size_t operator()(std::string_view sv) const noexcept { return std::hash<std::string_view>{}(sv); }
};

struct transparent_string_equal {
    using is_transparent = void;

    bool operator()(std::string_view a, std::string_view b) const noexcept { return a == b; }
};

}  // namespace core
