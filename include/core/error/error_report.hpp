/**
 * @file include/core/error/error_report.hpp
 * @brief Система диагностики и сообщений об ошибках.
 * @ingroup Core
 *
 * @defgroup CoreError Обработка ошибок
 * @brief Единая система сообщений об ошибках всех этапов компиляции.
 */

#pragma once
#include "core/error/error_codes.hpp"
#include "core/token/token.hpp"

#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace core {

/**
 * @brief Централизованный обработчик ошибок интерпретатора.
 * @ingroup CoreError
 *
 * Аккумулирует ошибки всех этапов обработки:
 * - Лексический анализ (неожиданные символы, незакрытые строки)
 * - Синтаксический анализ (пропущенные скобки, неверные выражения)
 * - Семантический анализ (несоответствие типов, необъявленные переменные)
 * - Время выполнения (деление на ноль, выход за границы)
 *
 * Для каждой ошибки выводит:
 * - Позицию в исходном коде (строка:колонка)
 * - Уровень (Error)
 * - Форматированное сообщение с контекстом
 * - Визуальное выделение проблемного места в исходном коде
 *
 * Особенности:
 * - **Форматирование сообщений**: через std::vformat с плейсхолдерами {}
 * - **Визуализация ошибки**: показывает строку исходного кода и позицию ошибки
 * - **Накопление ошибок**: не останавливается на первой, собирает все за проход
 * - **Фатальные ошибки**: parse_error и interpret_error прерывают выполнение
 *
 * @invariant has_error() возвращает true, если была хотя бы одна ошибка.
 *
 */
class error_reporter {
public:
    /**
     * @brief Конструктор.
     *
     * @param source Исходный код программы (для визуализации ошибок).
     *               Если не указан — визуализация отключена.
     */
    error_reporter(std::string_view source = {});

    /**
     * @brief Сообщает об ошибке с указанием позиции.
     *
     * @tparam Args Типы аргументов для форматирования
     * @param loc  Позиция в исходном коде
     * @param code Код ошибки
     * @param args Аргументы для подстановки в сообщение (плейсхолдеры {})
     *
     * @note Не прерывает выполнение — ошибка добавляется в список,
     *       выполнение продолжается для обнаружения других ошибок.
     */
    template <typename... Args>
    void error(location loc, error_code code, Args&&... args) {
        had_error_ = true;
        report(loc, "Error", format_message(code, std::forward<Args>(args)...));
    }

    /**
     * @brief Сообщает об ошибке, беря позицию из токена.
     *
     * Удобная перегрузка, когда позиция ошибки привязана к конкретному токену.
     *
     * @tparam T    Тип, имеющий поле loc_ (token, expression, statement)
     * @tparam Args Типы аргументов для форматирования
     * @param t    Объект с позицией (токен, узел AST)
     * @param code Код ошибки
     * @param args Аргументы для форматирования
     */
    template <typename T, typename... Args>
    void error(const T& t, error_code code, Args&&... args) {
        error(t.loc_, code, std::forward<Args>(args)...);
    }

    /**
     * @brief Сообщает о фатальной ошибке времени выполнения.
     *
     * Выводит сообщение и выбрасывает core::interpret_error.
     * Используется интерпретатором при невозможности продолжить выполнение.
     *
     * @tparam T    Тип с полем loc_
     * @tparam Args Типы аргументов для форматирования
     * @param t    Объект с позицией
     * @param code Код ошибки
     * @param args Аргументы для форматирования
     * @throws core::interpret_error Всегда.
     */
    template <typename T, typename... Args>
    [[noreturn]] void interpret_error(const T& t, error_code code, Args&&... args) {
        error(t.loc_, code, std::forward<Args>(args)...);
        throw core::interpret_error{code};
    }

    /**
     * @brief Сообщает о фатальной синтаксической ошибке.
     *
     * Выводит сообщение и выбрасывает core::parse_error.
     * Используется парсером при невозможности восстановления.
     *
     * @param token Токен, на котором обнаружена ошибка
     * @param code  Код ошибки
     * @throws core::parse_error Всегда.
     */
    [[noreturn]] void parse_error(const token& token, error_code code) {
        error(token.loc_, code);
        throw core::parse_error{};
    }

    /**
     * @brief Проверяет, были ли ошибки.
     *
     * Используется после каждого этапа для определения,
     * можно ли переходить к следующему.
     *
     * @return true, если зарегистрирована хотя бы одна ошибка.
     */
    bool has_error() const noexcept;

private:
    /// Разбирает исходный код на строки для визуализации ошибок.
    void build_line_cache();

    /**
     * @brief Выводит сообщение об ошибке с визуализацией.
     *
     * Формат вывода:
     * - [строка:колонка] Уровень: Сообщение
     * - Строка исходного кода с номером
     * - Маркер ^~~~ под проблемным местом
     *
     * @param loc  Позиция ошибки
     * @param kind Уровень ("Error")
     * @param msg  Текст сообщения
     */
    void report(location loc, std::string_view kind, std::string_view msg);

    /**
     * @brief Форматирует сообщение об ошибке.
     *
     * Подставляет аргументы в шаблон сообщения из error_table.
     * Если аргументов нет — возвращает сообщение как есть.
     *
     * @tparam Args Типы аргументов
     * @param code Код ошибки
     * @param args Аргументы для форматирования
     * @return Отформатированная строка.
     */
    template <typename... Args>
    std::string format_message(error_code code, Args&&... args) {
        auto format = get_error_message(code);
        if constexpr (sizeof...(Args) == 0) return std::string{format};
        return std::vformat(format, std::make_format_args(args...));
    }

    bool had_error_ = false;               ///< Была ли хотя бы одна ошибка
    std::string_view source_;              ///< Исходный код (для визуализации)
    std::vector<std::string_view> lines_;  ///< Кеш строк исходного кода
};

}  // namespace core
