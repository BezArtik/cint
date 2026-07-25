/**
 * @file include/core/value/operations.hpp
 * @brief Операции над значениями времени выполнения.
 * @ingroup CoreValue
 */

#pragma once
#include "core/value/value.hpp"

namespace core::ops {

/**
 * @defgroup CoreOps Операции над значениями
 * @brief Реализация арифметических, логических и битовых операций над core::value.
 * @ingroup CoreValue
 *
 * Все операции работают с универсальным типом core::value и автоматически
 * выполняют приведение типов по правилам языка:
 * - Арифметика: int + int → int, иначе → double
 * - Деление: проверка деления на ноль (выбрасывает interpret_error)
 * - Сравнение: числовые операнды, результат — bool
 * - Логика: операнды — bool, результат — bool
 *
 * @note Операции **не изменяют** переданные значения — всегда возвращают новый value.
 */

/// @name Арифметические операции
/// @ingroup CoreOps
/// @{

value add(const value& a, const value& b);  ///< Сложение: a + b
value sub(const value& a, const value& b);  ///< Вычитание: a - b
value mul(const value& a, const value& b);  ///< Умножение: a * b

/**
 * @brief Деление: a / b.
 *
 * Для целых операндов выполняет целочисленное деление (отбрасывание остатка).
 * Для остальных — деление с плавающей точкой.
 *
 * @throws core::interpret_error с кодом division_by_zero при b == 0.
 */
value div(const value& a, const value& b);

/**
 * @brief Остаток от деления: a % b.
 *
 * Работает только с целыми операндами. Знак результата совпадает со знаком a.
 *
 * @throws core::interpret_error с кодом modulo_by_zero при b == 0.
 */
value mod(const value& a, const value& b);

value unary_minus(const value& a);  ///< Унарный минус: -a

/// @}

/// @name Операции сравнения
/// @ingroup CoreOps
/// @{

value eq(const value& a, const value& b);   ///< Равно: a == b
value neq(const value& a, const value& b);  ///< Не равно: a != b
value lt(const value& a, const value& b);   ///< Меньше: a < b
value le(const value& a, const value& b);   ///< Меньше или равно: a <= b
value gt(const value& a, const value& b);   ///< Больше: a > b
value ge(const value& a, const value& b);   ///< Больше или равно: a >= b

/// @}

/// @name Логические операции
/// @ingroup CoreOps
/// @{

value and_op(const value& a, const value& b);  ///< Логическое И: a && b
value or_op(const value& a, const value& b);   ///< Логическое ИЛИ: a || b
value not_op(const value& a);                  ///< Логическое НЕ: !a

/// @}

/// @name Битовые операции
/// @ingroup CoreOps
/// @{

value bit_and(const value& a, const value& b);  ///< Побитовое И: a & b
value bit_or(const value& a, const value& b);   ///< Побитовое ИЛИ: a | b
value bit_xor(const value& a, const value& b);  ///< Побитовое исключающее ИЛИ: a ^ b
value bit_not(const value& a);                  ///< Побитовое НЕ: ~a

value shl(const value& a, const value& b);  ///< Сдвиг влево: a << b
value shr(const value& a, const value& b);  ///< Сдвиг вправо: a >> b

/// @}

}  // namespace core::ops
