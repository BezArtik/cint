// core/token/keywords.hpp


#pragma once
#include "core/token/token_types.hpp"
#include <cstdint>
#include <string_view>
#include <array>
#include <optional>

namespace core {

struct keyword_info {
    std::string_view lexeme_;
    type semantic_type_;
    bool is_type_;
    bool can_start_statement_;
};

inline const std::array keyword_table{
    keyword_info{"if",      type::void_type(),   false, true},
    keyword_info{"else",    type::void_type(),   false, false},
    keyword_info{"while",   type::void_type(),   false, true},
    keyword_info{"for",     type::void_type(),   false, true},
    keyword_info{"return",  type::void_type(),   false, true},

    keyword_info{"int",     type::int_type(),    true,  true},
    keyword_info{"double",  type::double_type(), true,  true},
    keyword_info{"bool",    type::bool_type(),   true,  true},
    keyword_info{"string",  type::string_type(), true,  true},
    keyword_info{"void",    type::void_type(),   true,  true},

    keyword_info{"true",    type::bool_type(),   false, false},
    keyword_info{"false",   type::bool_type(),   false, false},
};

std::optional<keyword_info> lookup_keyword(std::string_view lexeme);

} // namespace core
