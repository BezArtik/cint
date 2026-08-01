/**
 * @file include/core/token/keywords.hpp
 * @brief Таблица ключевых слов языка.
 * @ingroup CoreToken
 */

#pragma once
#include "core/token/token_types.hpp"
#include "core/token/type.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <string_view>

namespace core {

/**
 * @brief Информация о ключевом слове.
 * @ingroup CoreToken
 *
 * Связывает лексему ключевого слова с:
 * - Типом токена (для лексера)
 * - Семантическим типом (для парсера, при использовании как спецификатора типа)
 * - Флагами is_type_ и can_start_statement_ (для парсера)
 */
struct keyword_info {
    std::string_view lexeme_;   ///< Текст ключевого слова (напр. "if")
    token_type type_;           ///< Тип токена (напр. KW_IF)
    type semantic_type_;        ///< Семантический тип (для type-ключевых слов)
    bool is_type_;              ///< Является ли спецификатором типа (int, double, ...)
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
inline const std::array keyword_table{
    // Управляющие конструкции
    keyword_info{"if", token_type::KW_IF, type::void_type(), false, true},
    keyword_info{"else", token_type::KW_ELSE, type::void_type(), false, false},
    keyword_info{"while", token_type::KW_WHILE, type::void_type(), false, true},
    keyword_info{"for", token_type::KW_FOR, type::void_type(), false, true},
    keyword_info{"return", token_type::KW_RETURN, type::void_type(), false, true},

    // Спецификаторы типов
    keyword_info{"int", token_type::KW_INT, type::int_type(), true, true},
    keyword_info{"double", token_type::KW_DOUBLE, type::double_type(), true, true},
    keyword_info{"bool", token_type::KW_BOOL, type::bool_type(), true, true},
    keyword_info{"string", token_type::KW_STRING, type::string_type(), true, true},
    keyword_info{"void", token_type::KW_VOID, type::void_type(), true, true},

    // Структуры
    keyword_info{"struct", token_type::KW_STRUCT, type::void_type(), false, true},

    // Литералы
    keyword_info{"true", token_type::KW_TRUE, type::bool_type(), false, false},
    keyword_info{"false", token_type::KW_FALSE, type::bool_type(), false, false},
};

/**
 * @brief Ищет ключевое слово по лексеме.
 *
 * Вызывается лексером после чтения идентификатора.
 * Если лексема не является ключевым словом — возвращает IDENTIFIER.
 *
 * @param lexeme Текст идентификатора
 * @return Тип токена (KW_* или IDENTIFIER)
 */
inline token_type lookup_keyword(std::string_view lexeme) {
    auto&& it = std::ranges::find(keyword_table, lexeme, &keyword_info::lexeme_);
    return it != keyword_table.end() ? it->type_ : token_type::IDENTIFIER;
}

/**
 * @brief Возвращает информацию о ключевом слове по типу токена.
 *
 * Используется парсером для получения семантического типа
 * при разборе спецификаторов типа (int → type::int_type()).
 *
 * @param t Тип токена (должен быть ключевым словом)
 * @return Информация о ключевом слове.
 * @pre t — одно из KW_* значений.
 */
inline const keyword_info& get_keyword_info(token_type t) {
    auto&& it = std::ranges::find(keyword_table, t, &keyword_info::type_);
    assert(it != keyword_table.end());
    return *it;
}

/**
 * @brief Проверяет, может ли токен начинать инструкцию.
 *
 * Используется в parser::synchronize() для определения точек
 * восстановления после синтаксической ошибки.
 *
 * @param type Тип токена
 * @return true, если токен может начинать новую инструкцию.
 */
inline bool is_statement_start(token_type type) {
    if (type == token_type::LEFT_BRACE) return true;
    auto&& it = std::ranges::find(keyword_table, type, &keyword_info::type_);
    return it != keyword_table.end() && it->can_start_statement_;
}

}  // namespace core
