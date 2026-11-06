// core/error/diagnostic_renderer.cpp

#include "core/error/diagnostic_renderer.hpp"

#include <print>

namespace cint::core {

void diagnostic_renderer::build_line_cache() {
    if (!lines_.empty()) return;
    if (source_.empty()) return;

    auto start = std::size_t{};
    while (start < source_.size()) {
        auto&& end = source_.find('\n', start);
        if (end == std::string_view::npos) {
            lines_.push_back(source_.substr(start));
            break;
        }
        lines_.push_back(source_.substr(start, end - start));
        start = end + 1;
    }
}

void diagnostic_renderer::render(const diagnostic& d) {
    std::println(out_, "[line {}:{}] Error: {}", d.loc_.line_, d.loc_.column_, d.message_);

    if (lines_.empty()) return;
    if (d.loc_.line_ == 0 || d.loc_.line_ - 1 >= lines_.size()) return;

    auto&& line = lines_[d.loc_.line_ - 1];
    if (line.empty()) return;

    constexpr auto max_width = std::size_t{80};
    auto start = std::size_t{};

    if (line.size() > max_width && d.loc_.column_ > max_width / 2) {
        start = d.loc_.column_ - max_width / 2;
        if (auto space = line.rfind(' ', start); space != std::string_view::npos) start = space + 1;
        if (start + max_width > line.size()) start = line.size() - max_width;
    }

    const auto display = line.substr(start, max_width);
    const auto caret_col = d.loc_.column_ - 1 - start;

    std::println(out_, "  {:>4} | {}", d.loc_.line_, display);

    if (caret_col < display.size()) {
        auto end = display.find_first_of(" ;()\t\n[]{},.", caret_col + 1);
        if (end == std::string_view::npos) end = display.size();
        const auto tilde_count = end - caret_col - 1;

        std::print(out_, "       | {:{}}{}", "", caret_col, '^');
        if (tilde_count > 0) std::print(out_, "{:~<{}}", "", tilde_count);
        std::println(out_);
    }
}

}  // namespace cint::core
