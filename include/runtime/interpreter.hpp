/**
 * @file include/runtime/interpreter.hpp
 * @brief Интерпретатор времени выполнения — исполнение AST.
 * @ingroup Runtime
 *
 * @defgroup Runtime Выполнение программы
 * @brief Исполнение AST после статического анализа.
 */

#pragma once
#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/builtins/builtins.hpp"
#include "core/error/error_report.hpp"
#include "core/symbol/symbol_registry.hpp"
#include "core/utils/scoped_map.hpp"
#include "core/value/value.hpp"
#include "debug/debug_writer.hpp"

#include <span>

/**
 * @brief Интерпретатор, выполняющий программу путём обхода AST.
 * @ingroup Runtime
 *
 * Реализует **пошаговое выполнение** абстрактного синтаксического дерева
 * без компиляции в промежуточный код. Каждому типу узла AST соответствует
 * метод execute_* (для инструкций) или evaluate_* (для выражений).
 *
 * Основные компоненты:
 * - **Области видимости**: core::scoped_map<core::value> для переменных.
 *   Автоматически создаются/уничтожаются при входе/выходе из блоков.
 * - **Вызов функций**: рекурсивный вызов с передачей аргументов по значению.
 *   Поддержка пользовательских функций и builtin-функций.
 * - **Контроль рекурсии**: ограничение глубины вызовов (MAX_RECURSION_DEPTH = 250).
 * - **Трассировка**: через debug_writer можно включить вывод состояния
 *   на каждом шаге выполнения.
 *
 * Обработка ошибок:
 * - Ошибки времени выполнения (деление на ноль, выход за границы массива)
 *   выбрасывают core::runtime_error и прекращают выполнение.
 *
 *
 * @note Интерпретатор **не проверяет типы** — предполагается, что
 *       семантический анализ (type_checker) уже выполнен успешно.
 *
 * @invariant После interpret() значения переменных не сохраняются.
 *            Каждый вызов интерпретатора начинает выполнение с чистого состояния.
 */
class interpreter {
public:
    /**
     * @brief Конструктор.
     *
     * @param reporter Обработчик ошибок времени выполнения
     * @param registry Реестр символов (функции, builtin-функции, структуры)
     * @param writer   Отладочный writer для трассировки выполнения.
     *                 Если не указан — трассировка отключена.
     */
    interpreter(core::error_reporter& reporter, const core::symbol_registry& registry,
                const debug::debug_writer& writer = {})
        : reporter_{reporter}, registry_{registry}, writer_{writer} {}
    /**
     * @brief Выполняет программу.
     *
     * Последовательно исполняет все инструкции верхнего уровня.
     * Пропускает объявления функций (func_declaration) — они выполняются
     * только при вызове.
     *
     * @param statements Список AST-узлов верхнего уровня
     *
     * @throws core::runtime_error при фатальной ошибке времени выполнения.
     *
     * @note Не вызывает функции автоматически. Точка входа — первая
     *       исполняемая инструкция верхнего уровня (не func_declaration).
     */
    void interpret(std::span<const ast::statement> statements);

private:
    /// @brief Результат выполнения инструкции.
    struct execution_result;

    execution_result execute(const ast::statement& stmt);
    execution_result execute(const ast::expression_stmt& stmt);
    execution_result execute(const ast::var_declaration_stmt& stmt);
    execution_result execute(const ast::func_declaration_stmt&);
    execution_result execute(const ast::struct_declaration_stmt&);
    execution_result execute(const ast::block_stmt& stmt);
    execution_result execute(const ast::while_stmt& stmt);
    execution_result execute(const ast::for_stmt& stmt);
    execution_result execute(const ast::if_stmt& stmt);
    execution_result execute(const ast::return_stmt& stmt);

    core::value evaluate(const ast::expression& expr);
    core::value evaluate(const ast::variable_expr& expr);
    core::value evaluate(const ast::literal_expr& expr);
    core::value evaluate(const ast::assignment_expr& expr);
    core::value evaluate(const ast::binary_expr& expr);
    core::value evaluate(const ast::unary_expr& expr);
    core::value evaluate(const ast::postfix_expr& expr);
    core::value evaluate(const ast::index_expr& expr);
    core::value evaluate(const ast::member_access_expr& expr);
    core::value evaluate(const ast::call_expr& expr);
    core::value evaluate(const ast::initializer_list_expr& expr);

    /**
     * @brief Вычисляет lvalue-ссылку на переменную.
     *
     * Используется для изменения значения по месту. В отличие от evaluate(),
     * возвращает ссылку на value, хранящуюся в таблице переменных.
     *
     * @param expr lvalue-выражение
     * @return Ссылка на значение переменной.
     * @throws core::runtime_error если выражение не является lvalue.
     */
    core::value& evaluate_lvalue(const ast::expression& expr);
    core::value& evaluate_lvalue(const ast::variable_expr& expr);
    core::value& evaluate_lvalue(const ast::index_expr& expr);
    core::value& evaluate_lvalue(const ast::member_access_expr& expr);

    core::error_reporter& reporter_;         ///< Обработчик ошибок
    const core::symbol_registry& registry_;  ///< Реестр функций
    debug::debug_writer writer_;             ///< Отладочный вывод
    core::scoped_map<core::value> values_;   ///< Таблица переменных (стек областей видимости)
    uint32_t recursion_depth_ = 0;           ///< Текущая глубина рекурсии

    /// Максимально допустимая глубина рекурсивных вызовов.
    static constexpr uint32_t MAX_RECURSION_DEPTH = 250;
};
