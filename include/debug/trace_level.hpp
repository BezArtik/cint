// debug/trace_level.hpp
#pragma once

#include <cstdint>

namespace debug {

enum class trace_level : uint8_t {
    none = 0,
    tokens = 1 << 0,
    ast = 1 << 1,
    execution = 1 << 2,
    calls = 1 << 3,
    returns = 1 << 4,

    all = tokens | ast | execution | calls | returns
};

constexpr trace_level operator|(trace_level a, trace_level b) noexcept {
    return static_cast<trace_level>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

constexpr trace_level operator&(trace_level a, trace_level b) noexcept {
    return static_cast<trace_level>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

constexpr bool has_level(trace_level mask, trace_level level) noexcept {
    return (mask & level) != trace_level::none;
}

}  // namespace debug
