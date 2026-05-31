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
    parse_error() : std::runtime_error("Parse error") {}
};

struct interpret_error : std::runtime_error {
    error_code code_;
    interpret_error(error_code c)
        : std::runtime_error("Interpret error"), code_(c) {}
};
} // namespace core