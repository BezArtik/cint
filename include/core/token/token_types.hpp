/**
 * @file include/core/token/token_types.hpp
 * @brief Перечисление всех типов токенов языка.
 * @ingroup CoreToken
 */

#pragma once
#include <array>
#include <cstdint>
#include <string_view>

namespace core {

/**
 * @def TOKEN_TYPES
 * @brief X-макрос для генерации перечисления токенов и таблицы имён.
 *
 * Каждый токен определён одним вызовом X(name). Это позволяет:
 * - Автоматически сгенерировать enum token_type
 * - Автоматически сгенерировать таблицу строковых имён token_type_names
 * - Избежать рассинхронизации между enum и таблицей имён
 *
 * Категории токенов:
 * - **Разделители**: скобки, запятая, точка, точка с запятой
 * - **Арифметические операторы**: +, -, *, /, %
 * - **Операторы сравнения**: ==, !=, <, <=, >, >=
 * - **Составные присваивания**: +=, -=, *=, /=, %=
 * - **Битовые операторы**: &, |, ^, ~, <<, >>
 * - **Логические операторы**: &&, ||
 * - **Инкремент/декремент**: ++, --
 * - **Литералы и идентификаторы**: IDENTIFIER, STRING, NUMBER
 * - **Ключевые слова**: KW_IF, KW_WHILE, KW_FOR, ...
 * - **Служебные**: END_OF_FILE, UNKNOWN
 */

#define TOKEN_TYPES(X)           \
    /* Разделители */            \
    X(LEFT_PAREN)                \
    X(RIGHT_PAREN)               \
    X(LEFT_BRACE)                \
    X(RIGHT_BRACE)               \
    X(LEFT_BRACKET)              \
    X(RIGHT_BRACKET)             \
    X(COMMA)                     \
    X(DOT)                       \
    X(SEMICOLON)                 \
    /* Арифметика */             \
    X(PLUS)                      \
    X(MINUS)                     \
    X(STAR)                      \
    X(SLASH)                     \
    X(PERCENT)                   \
    /* Унарные */                \
    X(BANG)                      \
    /* Присваивание */           \
    X(EQUAL)                     \
    X(BANG_EQUAL)                \
    X(EQUAL_EQUAL)               \
    /* Сравнения */              \
    X(GREATER)                   \
    X(GREATER_EQUAL)             \
    X(LESS)                      \
    X(LESS_EQUAL)                \
    /* Инкремент/декремент */    \
    X(INCREMENT)                 \
    X(DECREMENT)                 \
    /* Составные присваивания */ \
    X(PLUS_EQUAL)                \
    X(MINUS_EQUAL)               \
    X(STAR_EQUAL)                \
    X(SLASH_EQUAL)               \
    X(PERCENT_EQUAL)             \
    /* Битовые */                \
    X(BIT_AND)                   \
    X(BIT_OR)                    \
    X(XOR)                       \
    X(BIT_NOT)                   \
    X(SHL)                       \
    X(SHR)                       \
    X(BIT_AND_EQUAL)             \
    X(BIT_OR_EQUAL)              \
    X(XOR_EQUAL)                 \
    X(BIT_NOT_EQUAL)             \
    X(SHL_EQUAL)                 \
    X(SHR_EQUAL)                 \
    /* Логические */             \
    X(LOGICAL_AND)               \
    X(LOGICAL_OR)                \
    /* Литералы */               \
    X(IDENTIFIER)                \
    X(STRING)                    \
    X(NUMBER)                    \
    /* Ключевые слова */         \
    X(KW_IF)                     \
    X(KW_ELSE)                   \
    X(KW_WHILE)                  \
    X(KW_FOR)                    \
    X(KW_RETURN)                 \
    X(KW_TRUE)                   \
    X(KW_FALSE)                  \
    X(KW_INT)                    \
    X(KW_DOUBLE)                 \
    X(KW_BOOL)                   \
    X(KW_STRING)                 \
    X(KW_STRUCT)                 \
    X(KW_VOID)                   \
    /* Служебные */              \
    X(END_OF_FILE)               \
    X(UNKNOWN)

/**
 * @brief Перечисление всех типов токенов.
 * @ingroup CoreToken
 *
 * Генерируется X-макросом TOKEN_TYPES. Каждое значение соответствует
 * одной лексеме языка. Используется везде: лексером, парсером, отладкой.
 */
enum class token_type : uint8_t {
#define X(name) name,
    TOKEN_TYPES(X)
#undef X
};

/**
 * @brief Таблица строковых имён токенов для отладки.
 *
 * Индекс в массиве соответствует значению enum token_type.
 * Используется в debug::print_tokens() и сообщениях об ошибках.
 *
 */
inline constexpr std::array token_type_names = {
#define X(name) std::string_view(#name),
    TOKEN_TYPES(X)
#undef X
};

}  // namespace core
