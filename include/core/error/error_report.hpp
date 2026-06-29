// core/error/error_report.hpp

#pragma once
#include "core/error/error_codes.hpp"
#include "core/token/token.hpp"

#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace core {

class error_reporter {
public:
    error_reporter(std::string_view source = {});

    template <typename... Args>
    void error(location loc, error_code code, Args&&... args) {
        had_error_ = true;
        report(loc, "Error", format_message(code, std::forward<Args>(args)...));
    }

    template <typename T, typename... Args>
    void error(const T& t, core::error_code code, Args&&... args) {
        error(t.loc_, code, std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    [[noreturn]] void interpret_error(const T& t, core::error_code code, Args&&... args) {
        error(t.loc_, code, std::forward<Args>(args)...);
        throw core::interpret_error{code};
    }

    [[noreturn]] void parse_error(const core::token& token, core::error_code code) {
        error(token.loc_, code);
        throw core::parse_error{};
    }

    bool has_error() const noexcept;

private:
    bool had_error_ = false;
    std::string_view source_;
    std::vector<std::string_view> lines_;

    void build_line_cache();

    void report(location loc, std::string_view kind, std::string_view msg);

    template <typename... Args>
    std::string format_message(error_code code, Args&&... args) {
        auto format = get_error_message(code);
        if constexpr (sizeof...(Args) == 0) return std::string{format};
        return std::vformat(format, std::make_format_args(args...));
    }
};

}  // namespace core
