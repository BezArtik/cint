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
#include "core/error/error_report.hpp"
#include "core/utils/builtins.hpp"
#include "core/utils/scoped_map.hpp"
#include "core/utils/symbol_registry.hpp"
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
 * - **Области видимости**: core::scoped_map<runtime_var> для переменных.
 *   Автоматически создаются/уничтожаются при входе/выходе из блоков.
 * - **Вызов функций**: рекурсивный вызов с передачей аргументов по значению.
 *   Поддержка пользовательских функций и builtin-функций.
 * - **Контроль рекурсии**: ограничение глубины вызовов (MAX_RECURSION_DEPTH = 250).
 * - **Трассировка**: через debug_writer можно включить вывод состояния
 *   на каждом шаге выполнения.
 *
 * Обработка ошибок:
 * - Ошибки времени выполнения (деление на ноль, выход за границы массива)
 *   выбрасывают core::interpret_error и прекращают выполнение.
 * - После перехвата interpret_error дальнейшее выполнение невозможно.
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
                const debug::debug_writer& writer = {});

    /**
     * @brief Выполняет программу.
     *
     * Последовательно исполняет все инструкции верхнего уровня.
     * Пропускает объявления функций (func_declaration) — они выполняются
     * только при вызове.
     *
     * @param statements Список AST-узлов верхнего уровня
     *
     * @throws core::interpret_error при фатальной ошибке времени выполнения.
     *
     * @note Не вызывает функции автоматически. Точка входа — первая
     *       исполняемая инструкция верхнего уровня (не func_declaration).
     */
    void interpret(std::span<const ast::node<ast::statement>> statements);

private:
    /// @brief Результат выполнения инструкции.
    struct execution_result;

    /// Диспетчеризует выполнение по типу инструкции.
    execution_result execute(const ast::statement& stmt);

    /// Вычисляет выражение и отбрасывает результат.
    execution_result execute_expression_stmt(const ast::expression_stmt& stmt);

    /// Создаёт переменную с начальным значением в текущей области видимости.
    execution_result execute_var_declaration(const ast::var_declaration& stmt);

    /**
     * @brief Выполняет блок инструкций.
     *
     * @param create_scope Если true — создаёт новую область видимости
     *                     для переменных, объявленных внутри блока.
     */
    execution_result execute_block(const ast::block_stmt& stmt, bool create_scope = true);

    /// Выполняет тело конструкции (блок или одиночную инструкцию).
    execution_result execute_body(const ast::statement& body);
    execution_result execute_while(const ast::while_stmt& stmt);
    execution_result execute_for(const ast::for_stmt& stmt);
    execution_result execute_if(const ast::if_stmt& stmt);

    /// Вычисляет возвращаемое значение и оборачивает в execution_result::return_.
    execution_result execute_return_stmt(const ast::return_stmt& stmt);

    /**
     * @brief Вычисляет значение выражения.
     *
     * Рекурсивно обходит дерево выражения и вычисляет результат.
     * При включённой трассировке выводит выражение и результат
     * через debug_writer.
     *
     * @param expr Выражение для вычисления
     * @return Значение выражения.
     */
    core::value evaluate(const ast::expression& expr);

    core::value evaluate_literal(const ast::literal_expr& expr);
    core::value evaluate_assignment(const ast::assignment_expr& expr);
    core::value evaluate_binary(const ast::binary_expr& expr);
    core::value evaluate_unary(const ast::unary_expr& expr);
    core::value evaluate_postfix(const ast::postfix_expr& expr);

    /**
     * @brief Вычисляет вызов функции.
     *
     * Поддерживает:
     * - **Пользовательские функции**: создаёт область видимости,
     *   связывает аргументы с параметрами, выполняет тело.
     * - **Builtin-функции**: вызывает нативную реализацию напрямую.
     *
     * Контролирует глубину рекурсии через MAX_RECURSION_DEPTH.
     */
    core::value evaluate_call(const ast::call_expr& expr);

    core::value evaluate_initializer_list(const ast::initializer_list_expr& expr);

    /**
     * @brief Вычисляет lvalue-ссылку на переменную.
     *
     * Используется в присваиваниях и инкрементах/декрементах для
     * изменения значения по месту. В отличие от evaluate(),
     * возвращает ссылку на value, хранящуюся в таблице переменных.
     *
     * @param expr lvalue-выражение
     * @return Ссылка на значение переменной.
     * @throws core::interpret_error если выражение не является lvalue.
     */
    core::value& evaluate_lvalue(const ast::expression& expr);

    core::error_reporter& reporter_;         ///< Обработчик ошибок
    const core::symbol_registry& registry_;  ///< Реестр функций
    const debug::debug_writer writer_;       ///< Отладочный вывод
    core::scoped_map<core::value> values_;  ///< Таблица переменных (стек областей видимости)
    uint32_t recursion_depth_ = 0;  ///< Текущая глубина рекурсии

    /// Максимально допустимая глубина рекурсивных вызовов.
    static constexpr uint32_t MAX_RECURSION_DEPTH = 250;
};
