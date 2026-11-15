// core/error/diagnostic_renderer.hpp
#pragma once
#include "core/error/diagnostic.hpp"

#include <concepts>
#include <iostream>
#include <iterator>
#include <string_view>
#include <vector>

namespace cint::core {

class diagnostic_renderer {
public:
    diagnostic_renderer(std::string_view source, std::ostream& out = std::cerr) : source_{source}, out_{out} {
        build_line_cache();
    }

    template <std::input_iterator It>
        requires std::convertible_to<std::iter_reference_t<It>, diagnostic>
    void render(It first, It last) {
        for (; first != last; ++first) render(*first);
    }

private:
    void render(const diagnostic& d);
    void build_line_cache();

    std::string_view source_;
    std::ostream& out_;
    std::vector<std::string_view> lines_;
};

}  // namespace cint::core
