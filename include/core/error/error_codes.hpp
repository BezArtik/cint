// core/error/error_codes.hpp


#pragma once
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <stdexcept>

namespace core {

enum class error_code : uint8_t {
#define ERROR(code, msg) code,
#include "core/error/error_codes.def"
#undef ERROR
};

struct error_message {
    std::string_view format_;
};

inline const std::unordered_map<error_code, error_message> error_messages = {
#define ERROR(code, msg) {error_code::code, {msg}},
#include "core/error/error_codes.def"
#undef ERROR
};

struct parse_error : std::runtime_error {
    error_code code_;
    size_t line_ = 0;
    size_t column_ = 0;

    parse_error(error_code c, size_t line = 0, size_t col = 0)
        : std::runtime_error("Parse error"), code_(c), line_(line), column_(col) {}
};

struct interpret_error : std::runtime_error {
    error_code code_;
    size_t line_ = 0;
    size_t column_ = 0;

    interpret_error(error_code c, size_t line = 0, size_t col = 0)
        : std::runtime_error("Interpret error"), code_(c), line_(line), column_(col) {}
};

} // namespace core