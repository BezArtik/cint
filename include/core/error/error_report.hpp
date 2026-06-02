// core/error/error_report.hpp


#pragma once
#include "core/error/error_codes.hpp"
#include <string_view>
#include <string>
#include <format>
#include <iostream>
#include <cstdint>

namespace core {

class error_reporter {
public:

    error_reporter(std::string_view source = {});

    template<typename... Args>
    void error(uint32_t line, uint32_t column, error_code code, Args&&... args) {
        had_error_ = true;
        auto msg = format_message(code, std::forward<Args>(args)...);
        report(line, column, "Error", msg);
    }

    bool has_error() const noexcept;

private:
    bool had_error_ = false;
    std::string_view source_;

    void report(uint32_t line, uint32_t column, std::string_view kind, const std::string& msg);

    template<typename... Args>
    static std::string format_message(error_code code, Args&&... args) {
        auto it = error_messages.find(code);
        if (it == error_messages.end()) return "Unknown error";
		if constexpr (sizeof...(Args) == 0) return std::string{ it->second.format_ };
        return std::vformat(it->second.format_, std::make_format_args(args...));
    }
};

} // namespace core