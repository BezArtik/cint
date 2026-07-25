/**
 * @file include/core/error/error_codes.hpp
 * @brief Коды ошибок и таблица сообщений.
 * @ingroup CoreError
 */

#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace core {

/**
 * @brief Коды всех возможных ошибок интерпретатора.
 * @ingroup CoreError
 *
 * Генерируется X-макросом из файла error_codes.def.
 * Каждый код соответствует одному виду ошибки.
 *
 */
enum class error_code : uint8_t {
#define ERROR(code, msg) code,
#include "core/error/error_codes.def"
#undef ERROR
};

/**
 * @brief Запись таблицы ошибок: код + шаблон сообщения.
 *
 * Шаблон может содержать плейсхолдеры {} для подстановки аргументов.
 * Пример: "Undefined variable '{}'" — {} будет заменён на имя переменной.
 */
struct error_entry {
    error_code code_;          ///< Код ошибки
    std::string_view format_;  ///< Шаблон сообщения (с опциональными {})
};

/**
 * @brief Таблица всех сообщений об ошибках.
 *
 * Индекс в массиве НЕ соответствует значению enum (для этого
 * используйте get_error_message с линейным поиском).
 *
 * Генерируется из error_codes.def.
 */
inline constexpr std::array error_table = {
#define ERROR(code, msg) error_entry{error_code::code, msg},
#include "core/error/error_codes.def"
#undef ERROR
};

/**
 * @brief Возвращает шаблон сообщения по коду ошибки.
 *
 * Линейный поиск по таблице. Вызывается при каждой ошибке,
 * но таблица маленькая (~50 записей) — не является узким местом.
 *
 * @param code Код ошибки
 * @return Шаблон сообщения или "Unknown error" для несуществующего кода.
 */
inline std::string_view get_error_message(error_code code) {
    auto it = std::ranges::find(error_table, code, &error_entry::code_);
    return it != error_table.end() ? it->format_ : "Unknown error";
}

/**
 * @brief Исключение: фатальная синтаксическая ошибка.
 *
 * Выбрасывается парсером, когда восстановление невозможно.
 * Перехватывается в parser::parse() для продолжения разбора
 * следующего объявления верхнего уровня.
 */
struct parse_error : std::exception {
    const char* what() const noexcept override { return "Syntax error"; }
};

/**
 * @brief Исключение: фатальная ошибка времени выполнения.
 *
 * Содержит код ошибки для возможной обработки.
 * Выбрасывается интерпретатором и операциями над значениями.
 * Перехватывается в interpreter::interpret() — выполнение прекращается.
 */
struct interpret_error : std::exception {
    error_code code_;  ///< Код ошибки для диагностики

    interpret_error(error_code c) noexcept : code_(c) {}
    const char* what() const noexcept override { return "Runtime error"; }
};

}  // namespace core
