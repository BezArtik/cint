// core/error/error_report.hpp


#pragma once
#include "core/error/error_codes.hpp"
#include "core/token/token.hpp"
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
        report(line, column, "Error", format_message(code, std::forward<Args>(args)...));
    }

    template<typename T, typename... Args>
    void error(const T& t, core::error_code code, Args&&... args) {
        had_error_ = true;
        report(t.line_, t.column_, "Error", format_message(code, std::forward<Args>(args)...));
    }

    template<typename T, typename... Args>
    [[nodiscard]] core::type error_type(const T& t, core::error_code code, Args&&... args) {
        had_error_ = true;
        report(t.line_, t.column_, "Error", format_message(code, std::forward<Args>(args)...));
        return core::type::unknown_type();
    }

    template<typename T, typename... Args>
    [[noreturn]] void error_throw(const T& t, core::error_code code, Args&&... args) {
        had_error_ = true;
        report(t.line_, t.column_, "Error", format_message(code, std::forward<Args>(args)...));
        throw core::interpret_error{ code };
    }

    [[noreturn]] void error(const core::token& token, core::error_code code) {
        had_error_ = true;
        report(token.line_, token.column_, "Error", format_message(code));
        throw core::parse_error{};
    }

    bool has_error() const noexcept;

private:
    bool had_error_ = false;
    std::string_view source_;

    void report(uint32_t line, uint32_t column, std::string_view kind, const std::string& msg);

    template<typename... Args>
    std::string format_message(error_code code, Args&&... args) {
        auto format = get_error_message(code);
        if constexpr (sizeof...(Args) == 0) return std::string{ format };
        return std::vformat(format, std::make_format_args(args...));
    }
};

} // namespace core