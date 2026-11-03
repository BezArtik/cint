/**
 * @file include/debug/debug_writer.hpp
 * @brief Конфигурируемый writer для отладочного вывода с методами печати.
 * @ingroup Debug
 *
 * @defgroup Debug Отладка и трассировка
 * @brief Инфраструктура для вывода отладочной информации о работе интерпретатора.
 */

#pragma once

#include "ast/node.hpp"
#include "core/token/token.hpp"
#include "core/value/value.hpp"

#include <cstdint>
#include <ostream>
#include <span>
#include <string_view>

namespace cint::debug {

/**
 * @brief Битовые флаги уровней трассировки.
 * @ingroup Debug
 *
 * Определяют, какая информация выводится при включённой отладке.
 * Поддерживается комбинирование через побитовые операции (|, &).
 */
enum class trace_level : std::uint8_t {
    none = 0,  ///< Трассировка отключена

    tokens = 1 << 0,     ///< Вывод токенов после лексического анализа
    ast = 1 << 1,        ///< Вывод AST после парсинга и перед выполнением
    execution = 1 << 2,  ///< Пошаговый вывод инструкций и выражений при выполнении
    returns = 1 << 3,    ///< Вывод возвращаемых значений из функций

    all = tokens | ast | execution | returns  ///< Все уровни сразу
};

/// @name Побитовые операции над trace_level
/// @{

inline trace_level operator|(trace_level a, trace_level b) noexcept {
    return static_cast<trace_level>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
}

inline trace_level operator&(trace_level a, trace_level b) noexcept {
    return static_cast<trace_level>(static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b));
}

inline bool has_level(trace_level mask, trace_level level) noexcept {
    return (mask & level) != trace_level::none;
}

/// @}

/**
 * @brief Настраиваемый обработчик отладочного вывода с методами печати.
 * @ingroup Debug
 *
 * Объединяет:
 * - **Политику вывода**: функция write_ (куда писать)
 * - **Фильтрацию**: маска mask_ (что писать)
 * - **Методы печати**: для токенов, AST, инструкций и выражений
 *
 * Управление отступами автоматическое через поле curr_level_ и RAII-guard.
 */
class [[nodiscard]] debug_writer {
public:
    /**
     * @brief Конструктор.
     *
     * @param write Поток вывода
     * @param mask  Маска активных уровней трассировки
     */
    debug_writer(std::ostream& os, trace_level mask) : os_{os}, mask_{mask} {}

    /**
     * @brief Проверяет, нужно ли выводить сообщение данного уровня.
     */
    bool enabled(trace_level level) const noexcept;

    /// @name Печать этапов компиляции
    /// @{

    /**
     * @brief Выводит список токенов после лексического анализа.
     * Требует trace_level::tokens.
     */
    void print_tokens(std::span<const core::token> tokens);

    /**
     * @brief Выводит абстрактное синтаксическое дерево.
     * Требует trace_level::ast.
     */
    void print_ast(std::span<const ast::statement> statements);

    /// @}

    /// @name Печать инструкций и выражений
    /// @{

    /**
     * @brief Выводит инструкцию.
     * Требует trace_level::ast или trace_level::execution.
     *
     * @param stmt   Инструкция для вывода
     * @param result Результат выполнения (если уже известен)
     */
    void print(const ast::statement& stmt, const core::value* result = nullptr);

    /**
     * @brief Выводит выражение.
     * Требует trace_level::ast или trace_level::execution.
     *
     * @param expr   Выражение для вывода
     * @param result Результат вычисления (если уже известен)
     */
    void print(const ast::expression& expr, const core::value* result = nullptr);

    /// @}

    /**
     * @brief Выводит информацию о возврате из функции.
     * Требует trace_level::returns.
     *
     * @param func_name Имя функции
     * @param result    Возвращённое значение
     */
    void print_return(std::string_view func_name, const core::value& result);

    /*
     * @brief Выводит информацию об использовании переменной.
     * Требует trace_level::execution.
     *
     * @param name Имя переменной
     * @param val  Значение переменной
     *
     */
    void print_var(std::string_view name, const core::value& val);

private:
    /// @name Внутренние перегрузки для инструкций
    /// @{

    void print(const ast::expression_stmt& stmt);
    void print(const ast::var_declaration_stmt& stmt);
    void print(const ast::block_stmt& stmt);
    void print(const ast::while_stmt& stmt);
    void print(const ast::for_stmt& stmt);
    void print(const ast::if_stmt& stmt);
    void print(const ast::return_stmt& stmt);
    void print(const ast::func_declaration_stmt& stmt);
    void print(const ast::struct_declaration_stmt& stmt);

    /// @}

    /// @name Внутренние перегрузки для выражений
    /// @{

    void print(const ast::literal_expr& expr);
    void print(const ast::variable_expr& expr);
    void print(const ast::binary_expr& expr);
    void print(const ast::assignment_expr& expr);
    void print(const ast::unary_expr& expr);
    void print(const ast::postfix_expr& expr);
    void print(const ast::call_expr& expr);
    void print(const ast::initializer_list_expr& expr);
    void print(const ast::index_expr& expr);
    void print(const ast::member_access_expr& expr);

    /// @}

    /**
     * @brief Создает строку с отступом в соответствии с текущим уровнем.
     */
    std::string indent() const;

    std::ostream& os_;         ///< Поток вывода
    trace_level mask_;         ///< Маска активных уровней
    uint32_t curr_level_ = 0;  ///< Текущий уровень отступа
};

}  // namespace cint::debug
