// debug/debug_writer.hpp
#pragma once

#include "debug/trace_level.hpp"

#include <functional>
#include <string_view>

namespace debug {

struct debug_writer {
    std::function<void(std::string_view)> write_;
    trace_level mask_ = trace_level::none;

    bool enabled(trace_level level) const noexcept { return write_ && has_level(mask_, level); }

    void emit(std::string_view msg) const {
        if (write_) write_(msg);
    }
};

}  // namespace debug
