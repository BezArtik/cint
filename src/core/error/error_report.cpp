// core/error/error_report.cpp


#include "core/error/error_report.hpp"
#include <algorithm>
#include <cctype>

namespace core {

error_reporter::error_reporter(std::string_view source): source_(source) {}

bool error_reporter::has_error() const noexcept {
	return had_error_;
}

void error_reporter::clear_errors() noexcept {
	had_error_ = false;
}

void error_reporter::report(size_t line, size_t column,
                            std::string_view kind, const std::string& msg) {
    std::cerr << std::format("[line {}:{}] {}: {}\n", line, column, kind, msg);

    if (source_.empty()) return;

    std::string_view source_line;
    size_t current_line = 1;
    size_t start = 0;

    for (size_t i = 0; i < source_.size(); ++i) {
        if (source_[i] == '\n') {
            if (current_line == line) {
                source_line = source_.substr(start, i - start);
                break;
            }
            current_line++;
            start = i + 1;
        }
    }

    if (current_line == line && source_line.empty()) {
        source_line = source_.substr(start);
    }

    if (source_line.empty()) return;

    constexpr size_t max_width = 80;
    size_t display_start = 0;

    if (source_line.size() > max_width && column > max_width / 2) {
        display_start = column - max_width / 2;
        while (display_start > 0 && !std::isspace(source_line[display_start - 1])) {
            display_start--;
        }
        source_line = source_line.substr(display_start);
        source_line = source_line.substr(0, std::min(source_line.size(), max_width));
    }

    std::cerr << std::format("  {:>4} | {}\n", line, source_line);

    size_t caret_pos = column - display_start - 1;
    if (caret_pos < source_line.size()) {
        std::string caret(source_line.size(), ' ');
        size_t underscore_start = caret_pos;
        size_t underscore_end = caret_pos + 1;

        while (underscore_end < source_line.size() &&
            !std::isspace(source_line[underscore_end]) &&
            source_line[underscore_end] != ';' &&
            source_line[underscore_end] != ')') {
            underscore_end++;
        }

        for (size_t i = 0; i < underscore_start; ++i) caret[i] = ' ';
        caret[underscore_start] = '^';
        for (size_t i = underscore_start + 1; i < underscore_end; ++i) caret[i] = '~';

        std::cerr << std::format("       | {}\n", caret);
    }
}

} // namespace core