/**
 * @file include/debug/debug.hpp
 * @brief Функции отладочной печати токенов, AST и выполнения.
 * @ingroup Debug
 */

#pragma once

#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/token/token.hpp"
#include "core/value/value.hpp"
#include "debug/debug_writer.hpp"

#include <cstdint>
#include <span>
#include <string_view>

namespace debug {

/**
 * @name Трассировка этапов компиляции
 * @ingroup Debug
 * @{
 */

/**
 * @brief Выводит список токенов после лексического анализа.
 *
 * Формат вывода: таблица с типом токена, лексемой и позицией.
 * Требует trace_level::tokens.
 *
 * @param writer Настроенный debug_writer
 * @param tokens Список токенов от лексера
 */
void print_tokens(const debug_writer& writer, std::span<const core::token> tokens);

/**
 * @brief Выводит абстрактное синтаксическое дерево.
 *
 * Рекурсивно обходит все инструкции верхнего уровня и выводит
 * их структуру с отступами. Требует trace_level::ast.
 *
 * @param writer     Настроенный debug_writer
 * @param statements Список инструкций верхнего уровня
 */
void print_ast(const debug_writer& writer, std::span<const ast::statement> statements);

/// @}

/**
 * @name Трассировка выполнения
 * @ingroup Debug
 * @{
 */

/**
 * @brief Выводит инструкцию при пошаговом выполнении.
 *
 * Используется интерпретатором перед выполнением каждой инструкции.
 * Требует trace_level::execution.
 *
 * @param writer      Настроенный debug_writer
 * @param stmt        Выполняемая инструкция
 * @param level       Уровень вложенности (для отступов)
 * @param exec_result Результат выполнения (если уже известен)
 */
void print_statement(const debug_writer& writer, const ast::statement& stmt, uint32_t level = 0,
                     const core::value* exec_result = nullptr);

/**
 * @brief Выводит выражение при вычислении.
 *
 * Используется интерпретатором при вычислении каждого выражения.
 * Требует trace_level::execution.
 *
 * @param writer      Настроенный debug_writer
 * @param expr        Вычисляемое выражение
 * @param level       Уровень вложенности (для отступов)
 * @param eval_result Результат вычисления (если уже известен)
 */
void print_expression(const debug_writer& writer, const ast::expression& expr, uint32_t level = 0,
                      const core::value* eval_result = nullptr);

/**
 * @brief Выводит значение (компактное представление).
 *
 * Используется для вывода промежуточных результатов.
 *
 * @param writer Настроенный debug_writer
 * @param val    Значение для вывода
 * @param indent Уровень отступа
 */
void print_value(const debug_writer& writer, const core::value& val, uint32_t indent = 0);

/// @}

/**
 * @name Трассировка вызовов функций
 * @ingroup Debug
 * @{
 */

/**
 * @brief Выводит информацию о возврате из функции.
 *
 * Формат: `[RETURN] func_name → result`
 * Требует trace_level::returns.
 *
 * @param writer    Настроенный debug_writer
 * @param func_name Имя функции
 * @param result    Возвращённое значение
 * @param level     Уровень вложенности
 */
void print_return(const debug_writer& writer, std::string_view func_name, const core::value& result,
                  uint32_t level = 0);

/// @}

}  // namespace debug
