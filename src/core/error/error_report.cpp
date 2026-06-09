// core/error/error_report.cpp


#include "core/error/error_report.hpp"
#include <algorithm>
#include <cctype>

namespace core {

error_reporter::error_reporter(std::string_view source): source_(source) {}

bool error_reporter::has_error() const noexcept {
	return had_error_;
}

void error_reporter::report(location loc, std::string_view kind, const std::string& msg) {
    std::cerr << std::format("[line {}:{}] {}: {}\n", loc.line_, loc.column_, kind, msg);

    if (source_.empty()) return;

    auto remaining = source_;
    for (uint32_t i = 1; i < loc.line_ && !remaining.empty(); ++i) {
        auto pos = remaining.find('\n');
        if (pos == std::string_view::npos) return;
        remaining = remaining.substr(pos + 1);
    }

    auto eof = remaining.find('\n');
    auto source_line = (eof != std::string_view::npos)
        ? remaining.substr(0, eof)
        : remaining;

    if (source_line.empty()) return;

    constexpr size_t max_width = 80;
    auto display_line = source_line;

    if (display_line.size() > max_width && loc.column_ > max_width / 2) {
        size_t display_start = loc.column_ - max_width / 2;

        if (display_start > 0) {
            auto space = display_line.rfind(' ', display_start - 1);
            display_start = (space != std::string_view::npos) ? space + 1 : 0;
        }

        display_line = display_line.substr(display_start, max_width);
    } else {
        display_line = display_line.substr(0, max_width);
    }

    std::cerr << std::format("  {:>4} | {}\n", loc.line_, display_line);

    auto caret_pos = loc.column_ - (source_line.data() - display_line.data()) - 1;
    if (caret_pos < display_line.size()) {
        auto token_end = display_line.find_first_of(" ;()\t\n", caret_pos + 1);
        if (token_end == std::string_view::npos) token_end = display_line.size();

        std::string caret(display_line.size(), ' ');
        caret[caret_pos] = '^';
        for (size_t i = caret_pos + 1; i < token_end; ++i) caret[i] = '~';
            
        std::cerr << std::format("       | {}\n", caret);
    }
}

} // namespace core