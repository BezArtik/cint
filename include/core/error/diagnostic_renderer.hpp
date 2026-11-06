/**
 * @file include/core/error/diagnostic_renderer.hpp
 * @brief Рендеринг диагностических сообщений с визуализацией.
 * @ingroup CoreError
 */

#pragma once
#include "core/error/diagnostic.hpp"

#include <iostream>
#include <iterator>
#include <string_view>
#include <vector>

namespace cint::core {

/**
 * @brief Отображает диагностики: шапка + строка исходника + маркер.
 *
 */
class diagnostic_renderer {
public:
    diagnostic_renderer(std::string_view source, std::ostream& out = std::cerr) : source_{source}, out_{out} {
        build_line_cache();
    }

    template <std::input_iterator It>
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
