// core/token/keywords.hpp

#pragma once
#include "core/token/token_types.hpp"
#include "core/token/type.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <string_view>

namespace core {

struct keyword_info {
    std::string_view lexeme_;
    token_type type_;
    type semantic_type_;
    bool is_type_;
    bool can_start_statement_;
};

inline const std::array keyword_table{
    keyword_info{"if", token_type::KW_IF, type::void_type(), false, true},
    keyword_info{"else", token_type::KW_ELSE, type::void_type(), false, false},
    keyword_info{"while", token_type::KW_WHILE, type::void_type(), false, true},
    keyword_info{"for", token_type::KW_FOR, type::void_type(), false, true},
    keyword_info{"return", token_type::KW_RETURN, type::void_type(), false, true},
    keyword_info{"int", token_type::KW_INT, type::int_type(), true, true},
    keyword_info{"double", token_type::KW_DOUBLE, type::double_type(), true, true},
    keyword_info{"bool", token_type::KW_BOOL, type::bool_type(), true, true},
    keyword_info{"string", token_type::KW_STRING, type::string_type(), true, true},
    keyword_info{"void", token_type::KW_VOID, type::void_type(), true, true},
    keyword_info{"struct", token_type::KW_STRUCT, type::void_type(), false, true},
    keyword_info{"true", token_type::KW_TRUE, type::bool_type(), false, false},
    keyword_info{"false", token_type::KW_FALSE, type::bool_type(), false, false},
};

inline token_type lookup_keyword(std::string_view lexeme) {
    auto it = std::ranges::find(keyword_table, lexeme, &keyword_info::lexeme_);
    return it != keyword_table.end() ? it->type_ : token_type::IDENTIFIER;
}

inline const keyword_info& get_keyword_info(token_type t) {
    auto it = std::ranges::find(keyword_table, t, &keyword_info::type_);
    assert(it != keyword_table.end());
    return *it;
}

inline bool is_statement_start(token_type type) {
    if (type == token_type::LEFT_BRACE) return true;
    auto it = std::ranges::find(keyword_table, type, &keyword_info::type_);
    return it != keyword_table.end() && it->can_start_statement_;
}

}  // namespace core
