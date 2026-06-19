// core/error/error_codes.hpp


#pragma once
#include <cstdint>
#include <string_view>
#include <array>
#include <algorithm>


namespace core {

enum class error_code : uint8_t {
#define ERROR(code, msg) code,
#include "core/error/error_codes.def"
#undef ERROR
};

struct error_entry {
    error_code code_;
    std::string_view format_;
};

inline constexpr std::array error_table = {
#define ERROR(code, msg) error_entry{error_code::code, msg},
#include "core/error/error_codes.def"
#undef ERROR
};

inline std::string_view get_error_message(error_code code) {
    auto it = std::ranges::find(error_table, code, &error_entry::code_);
    return it != error_table.end() ? it->format_ : "Unknown error";
}

struct parse_error : std::exception {
    const char* what() const noexcept override { return "Syntax error"; }
};

struct interpret_error : std::exception {
    error_code code_;
    interpret_error(error_code c) noexcept : code_(c) {}
    const char* what() const noexcept override { return "Runtime error"; }
};

} // namespace core
