/**
 * @file include/core/token/keywords.hpp
 * @brief Таблица ключевых слов языка.
 * @ingroup CoreToken
 */

#pragma once
#include "core/token/token_types.hpp"

#include <array>
#include <string_view>

namespace cint::core {

/**
 * @brief Информация о ключевом слове.
 * @ingroup CoreToken
 *
 */
struct keyword_info {
    std::string_view lexeme_;  ///< Текст ключевого слова (напр. "if")
    token_type type_;          ///< Тип токена (напр. KW_IF)
};

/**
 * @brief Таблица всех ключевых слов языка.
 * @ingroup CoreToken
 *
 */
inline constexpr std::array keywords{
    /// Управляющие конструкции
    keyword_info{"if", token_type::KW_IF},
    keyword_info{"else", token_type::KW_ELSE},
    keyword_info{"while", token_type::KW_WHILE},
    keyword_info{"for", token_type::KW_FOR},
    keyword_info{"return", token_type::KW_RETURN},

    /// Спецификаторы типов
    keyword_info{"int", token_type::KW_INT},
    keyword_info{"double", token_type::KW_DOUBLE},
    keyword_info{"bool", token_type::KW_BOOL},
    keyword_info{"string", token_type::KW_STRING},
    keyword_info{"void", token_type::KW_VOID},

    /// Структуры
    keyword_info{"struct", token_type::KW_STRUCT},

    /// Литералы
    keyword_info{"true", token_type::KW_TRUE},
    keyword_info{"false", token_type::KW_FALSE},
};

}  // namespace cint::core
