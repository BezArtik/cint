/**
 * @file include/debug/trace_level.hpp
 * @brief Уровни детализации отладочной трассировки.
 * @ingroup Debug
 *
 * @defgroup Debug Отладка и трассировка
 * @brief Инфраструктура для вывода отладочной информации о работе интерпретатора.
 */

#pragma once

#include <cstdint>

namespace debug {

/**
 * @brief Битовые флаги уровней трассировки.
 * @ingroup Debug
 *
 * Определяют, какая информация выводится при включённой отладке.
 * Поддерживается комбинирование через побитовые операции (|, &).
 *
 * Уровни независимы — можно включить только трассировку токенов
 * или одновременно AST и выполнение.
 *
 */
enum class trace_level : uint8_t {
    none = 0,  ///< Трассировка отключена

    tokens = 1 << 0,  ///< Вывод токенов после лексического анализа
    ast = 1 << 1,     ///< Вывод AST после парсинга и перед выполнением
    execution = 1 << 2,  ///< Пошаговый вывод инструкций и выражений при выполнении
    calls = 1 << 3,    ///< Вывод вызовов функций с аргументами
    returns = 1 << 4,  ///< Вывод возвращаемых значений из функций

    all = tokens | ast | execution | calls | returns  ///< Все уровни сразу
};

/// @name Побитовые операции над trace_level
/// @{

/**
 * @brief Объединение флагов трассировки.
 *
 * Позволяет включить несколько уровней одновременно:
 * trace_level::tokens | trace_level::ast
 */
constexpr trace_level operator|(trace_level a, trace_level b) noexcept {
    return static_cast<trace_level>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

/**
 * @brief Пересечение флагов трассировки.
 *
 * Используется для проверки, входит ли уровень в маску:
 * has_level(mask, level) → (mask & level) != none
 */
constexpr trace_level operator&(trace_level a, trace_level b) noexcept {
    return static_cast<trace_level>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

/**
 * @brief Проверяет, установлен ли флаг уровня в маске.
 *
 * @param mask  Маска активных уровней
 * @param level Проверяемый уровень
 * @return true, если уровень активен.
 */
constexpr bool has_level(trace_level mask, trace_level level) noexcept {
    return (mask & level) != trace_level::none;
}

/// @}

}  // namespace debug
