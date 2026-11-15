// core/error/diagnostic.hpp
#pragma once
#include "core/error/error_codes.hpp"
#include "core/token/token.hpp"

#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cint::core {

struct diagnostic {
    error_code code_;
    location loc_;
    std::string message_;
};

template <typename... Args>
[[nodiscard]] auto format_message(error_code code, Args&&... args) {
    const auto format = error_table[static_cast<std::size_t>(code)].second;
    if constexpr (sizeof...(Args) == 0) return std::string{format};
    return std::vformat(format, std::make_format_args(args...));
}

class diagnostic_sink {
public:
    template <typename... Args>
    void report(location loc, error_code code, Args&&... args) {
        items_.emplace_back(code, loc, format_message(code, std::forward<Args>(args)...));
    }

    void report_formatted(location loc, error_code code, std::string msg) {
        items_.emplace_back(code, loc, std::move(msg));
    }

    [[nodiscard]] bool has_errors() const noexcept { return !items_.empty(); }

    [[nodiscard]] bool has_error(error_code code) const noexcept {
        return std::ranges::find(items_, code, &diagnostic::code_) != items_.end();
    }

    [[nodiscard]] auto begin() const noexcept { return items_.begin(); }
    [[nodiscard]] auto end() const noexcept { return items_.end(); }

    void clear() noexcept { items_.clear(); }

private:
    std::vector<diagnostic> items_;
};

}  // namespace cint::core
