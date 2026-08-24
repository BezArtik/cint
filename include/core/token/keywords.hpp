/**
 * @file include/core/token/keywords.hpp
 * @brief Таблица ключевых слов языка.
 * @ingroup CoreToken
 */

#pragma once
#include "core/token/token_types.hpp"
#include "core/type/type.hpp"

#include <array>
#include <string_view>

namespace core {

/**
 * @brief Информация о ключевом слове.
 * @ingroup CoreToken
 *
 * Связывает лексему ключевого слова с:
 * - Типом токена (для лексера)
 * - Семантическим типом (для парсера, при использовании как спецификатора типа)
 * - Флагом can_start_statement_ (для парсера)
 */
struct keyword_info {
    std::string_view lexeme_;   ///< Текст ключевого слова (напр. "if")
    token_type type_;           ///< Тип токена (напр. KW_IF)
    type semantic_type_;        ///< Семантический тип (для type-ключевых слов)
    bool can_start_statement_;  ///< Может ли начинать инструкцию (if, while, for, ...)
};

/**
 * @brief Таблица всех ключевых слов языка.
 * @ingroup CoreToken
 *
 * Используется лексером для классификации идентификаторов:
 * если идентификатор совпадает с ключевым словом — создаётся
 * токен соответствующего типа вместо IDENTIFIER.
 *
 * Порядок в таблице не важен (поиск линейный, слов мало).
 */
inline const std::array keywords{
    // Управляющие конструкции
    keyword_info{"if", token_type::KW_IF, type::void_type(), true},
    keyword_info{"else", token_type::KW_ELSE, type::void_type(), false},
    keyword_info{"while", token_type::KW_WHILE, type::void_type(), true},
    keyword_info{"for", token_type::KW_FOR, type::void_type(), true},
    keyword_info{"return", token_type::KW_RETURN, type::void_type(), true},

    // Спецификаторы типов
    keyword_info{"int", token_type::KW_INT, type::int_type(), true},
    keyword_info{"double", token_type::KW_DOUBLE, type::double_type(), true},
    keyword_info{"bool", token_type::KW_BOOL, type::bool_type(), true},
    keyword_info{"string", token_type::KW_STRING, type::string_type(), true},
    keyword_info{"void", token_type::KW_VOID, type::void_type(), true},

    // Структуры
    keyword_info{"struct", token_type::KW_STRUCT, type::void_type(), true},

    // Литералы
    keyword_info{"true", token_type::KW_TRUE, type::bool_type(), false},
    keyword_info{"false", token_type::KW_FALSE, type::bool_type(), false},
};

}  // namespace core
