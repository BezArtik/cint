// app/application.hpp
#pragma once

#include "debug/debug_writer.hpp"

#include <string>

namespace cint::app {

class application {
public:
    [[nodiscard]] int run(int argc, char** argv);

private:
    std::string filename_;
    debug::trace_level trace_mask_ = debug::trace_level::none;
};

}  // namespace cint::app
