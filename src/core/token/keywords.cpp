// core/token/keywords.cpp


#include "core/token/keywords.hpp"
#include "core/token/token_types.hpp"
#include <algorithm>
#include <optional>
#include <string_view>

namespace core {

std::optional<keyword_info> lookup_keyword(std::string_view lexeme) {
    auto it = std::ranges::find(keyword_table, lexeme, &keyword_info::lexeme_);
    if (it != keyword_table.end()) return *it;
    return std::nullopt;
}

} // namespace core