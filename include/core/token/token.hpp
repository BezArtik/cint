/**
 * @file include/core/token/token.hpp
 * @brief Структуры токена и позиции в исходном коде.
 * @ingroup Core
 *
 * @defgroup CoreToken Токены
 * @brief Представление лексем — минимальных единиц исходного кода.
 */

#pragma once

#include "core/token/token_types.hpp"
#include "core/value/value.hpp"

#include <cstdint>
#include <optional>
#include <string_view>

namespace core {

/**
 * @brief Позиция в исходном коде (строка и колонка).
 * @ingroup CoreToken
 *
 * Нумерация строк и колонок начинается с 1.
 * Используется для сообщений об ошибках и отладки.
 */
struct location {
    uint32_t line_;    ///< Номер строки (начиная с 1)
    uint32_t column_;  ///< Номер колонки (начиная с 1)
};

/**
 * @brief Токен — минимальная значимая единица исходного кода.
 * @ingroup CoreToken
 *
 * Формируется лексером на этапе лексического анализа.
 * Каждый токен содержит:
 * - **Тип** — классификация лексемы (число, оператор, ключевое слово, ...)
 * - **Лексему** — исходный текст (string_view в исходную строку)
 * - **Позицию** — местоположение в исходном коде
 * - **Литеральное значение** — для чисел, строк, true/false (опционально)
 *
 * Литеральное значение вычисляется **на этапе лексера** (не во время выполнения):
 * - Числа: сразу парсятся в int64_t или double
 * - Строки: обрабатываются escape-последовательности
 * - true/false: преобразуются в bool
 *
 * Это позволяет:
 * - Не парсить числа повторно при вычислении выражений
 * - Обнаруживать ошибки формата чисел на раннем этапе
 *
 * @note Лексема (lexeme_) ссылается на исходную строку программы —
 *       время жизни токена ограничено временем жизни исходного кода.
 */
struct token {
    token_type type_;          ///< Тип токена
    std::string_view lexeme_;  ///< Исходный текст лексемы
    location loc_;             ///< Позиция в исходном коде
    std::optional<value> literal_value_;  ///< Вычисленное литеральное значение (для чисел, строк, true/false)

    /// Создаёт неинициализированный токен.
    token() = default;

    /**
     * @brief Создаёт токен с указанными параметрами.
     *
     * @param type  Тип токена
     * @param lex   Исходный текст лексемы
     * @param loc   Позиция в исходном коде
     * @param val   Литеральное значение (std::nullopt для не-литералов)
     */
    token(token_type type, std::string_view lex, location loc, std::optional<value> val = std::nullopt)
        : type_(type), lexeme_(lex), loc_(loc), literal_value_(val) {}
};

}  // namespace core
