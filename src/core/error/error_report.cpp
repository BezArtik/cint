// core/error/error_report.cpp

#include "core/error/error_report.hpp"

#include <cctype>
#include <iostream>
#include <string_view>

namespace core {

error_reporter::error_reporter(std::string_view source) : source_(source) {
    build_line_cache();
}

bool error_reporter::has_error() const noexcept {
    return had_error_;
}

void error_reporter::build_line_cache() {
    if (!lines_.empty()) return;
    if (source_.empty()) return;

    size_t start = 0;
    while (start < source_.size()) {
        auto end = source_.find('\n', start);
        if (end == std::string_view::npos) {
            lines_.push_back(source_.substr(start));
            break;
        }
        lines_.push_back(source_.substr(start, end - start));
        start = end + 1;
    }
}

void error_reporter::report(location loc, std::string_view kind, std::string_view msg) {
    std::cerr << std::format("[line {}:{}] {}: {}\n", loc.line_, loc.column_, kind, msg);

    if (lines_.empty()) return;
    if (loc.line_ - 1 >= lines_.size()) return;

    auto line = lines_[loc.line_ - 1];
    if (line.empty()) return;

    constexpr size_t max_width = 80;
    size_t start = 0;

    if (line.size() > max_width && loc.column_ > max_width / 2) {
        start = loc.column_ - max_width / 2;
        if (auto space = line.rfind(' ', start); space != std::string_view::npos) start = space + 1;
        if (start + max_width > line.size()) start = line.size() - max_width;
    }

    auto display = line.substr(start, max_width);
    auto caret_col = loc.column_ - 1 - start;

    std::cerr << std::format("  {:>4} | {}\n", loc.line_, display);

    if (caret_col < display.size()) {
        auto end = display.find_first_of(" ;()\t\n[]{},.", caret_col + 1);
        if (end == std::string_view::npos) end = display.size();
        auto tilde_count = end - caret_col - 1;

        std::cerr << std::format("       | {:{}}{}", "", caret_col, '^');
        if (tilde_count > 0) std::cerr << std::format("{:~<{}}", "", tilde_count);
        std::cerr << '\n';
    }
}

}  // namespace core
