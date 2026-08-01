/**
 * @file include/semantics/type_check.hpp
 * @brief Семантический анализ — проверка типов и видимости переменных.
 * @ingroup Semantics
 *
 * @defgroup Semantics Семантический анализ
 * @brief Статическая проверка корректности программы после построения AST.
 */

#pragma once
#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/error/error_report.hpp"
#include "core/utils/scoped_map.hpp"
#include "core/utils/symbol_registry.hpp"

#include <optional>
#include <span>

/**
 * @brief Модуль статической проверки типов.
 * @ingroup Semantics
 *
 * Выполняет обход AST и проверяет:
 * - Соответствие типов в присваиваниях и инициализациях
 * - Корректность операндов (арифметика только для чисел, логика только для bool)
 * - Существование объявленных переменных и функций
 * - Соответствие количества и типов аргументов при вызове функций
 * - Корректность return (внутри функции, соответствие типа)
 * - Отсутствие повторных объявлений в одной области видимости
 *
 * Реализует **обход в глубину** с отслеживанием области видимости
 * через core::scoped_map. При обнаружении ошибки продолжает анализ
 * для выявления максимального количества проблем за один проход.
 *
 * @invariant После успешного выполнения check() программа гарантированно
 *            не содержит ошибок типов, видимости или дублирования имён.
 *
 */
class type_checker {
public:
    /**
     * @brief Конструктор.
     *
     * @param reporter Обработчик ошибок (должен существовать всё время жизни)
     * @param registry Реестр символов, построенный symbol_registry::build()
     *
     * @note registry должен содержать информацию о всех функциях и структурах
     *       верхнего уровня, включая builtin-функции.
     */
    type_checker(core::error_reporter& reporter, const core::symbol_registry& registry);

    /**
     * @brief Выполняет полную проверку программы.
     *
     * Обходит все инструкции верхнего уровня и накапливает ошибки
     * в reporter. Не останавливается на первой ошибке.
     *
     * @param statements Список AST-узлов верхнего уровня
     * @return true, если ошибок не обнаружено.
     */
    bool check(std::span<const ast::node<ast::statement>> statements);

private:
    /// Диспетчеризует проверку по типу инструкции.
    void check_statement(const ast::statement& stmt);
    void check_expression_stmt(const ast::expression_stmt& stmt);
    void check_var_declaration(const ast::var_declaration& stmt);
    void check_block(const ast::block_stmt& stmt, bool create_scope = false);

    /**
     * @brief Проверяет тело конструкции (блок или одиночную инструкцию).
     *
     * Если тело — блок, содержащий объявления переменных, создаёт
     * новую область видимости. Оптимизация: если блок не содержит
     * объявлений (has_declarations), scope не создаётся.
     *
     * @param body Тело для проверки
     */
    void check_body(const ast::statement& body);
    void check_while(const ast::while_stmt& stmt);
    void check_for(const ast::for_stmt& stmt);
    void check_if(const ast::if_stmt& stmt);
    void check_return_stmt(const ast::return_stmt& stmt);
    void check_struct_declaration(const ast::struct_declaration& stmt);

    /**
     * @brief Проверяет объявление функции.
     *
     * Создаёт новую область видимости для параметров, устанавливает
     * curr_return_type_ для проверки return-инструкций внутри тела.
     * Проверяет отсутствие повторных объявлений.
     */
    void check_func_declaration(const ast::func_declaration& stmt);

    /**
     * @brief Вычисляет тип выражения.
     *
     * Основной метод вывода типов. Рекурсивно обходит дерево выражения
     * и возвращает результирующий тип. При обнаружении ошибки возвращает
     * unknown_type, но продолжает анализ.
     *
     * @param expr Выражение для анализа
     * @return Тип выражения или unknown_type при ошибке.
     */
    core::type type_of(const ast::expression& expr);

    core::type type_of_literal(const ast::literal_expr& expr);
    core::type type_of_variable(const ast::variable_expr& expr);
    core::type type_of_binary(const ast::binary_expr& expr);
    core::type type_of_assignment(const ast::assignment_expr& expr);
    core::type type_of_unary(const ast::unary_expr& expr);
    core::type type_of_postfix(const ast::postfix_expr& expr);
    core::type type_of_call(const ast::call_expr& expr);
    core::type type_of_initializer_list(const ast::initializer_list_expr& expr);
    core::type type_of_index(const ast::index_expr& expr);
    core::type type_of_member_access(const ast::member_access_expr& expr);

    /**
     * @brief Проверяет, является ли выражение lvalue.
     *
     * lvalue — выражение, которое может стоять слева от присваивания:
     * переменная, индексация массива, доступ к полю структуры.
     *
     * @param expr Выражение для проверки
     * @return true, если выражение можно использовать как lvalue.
     */
    bool is_lvalue(const ast::expression& expr);

    core::error_reporter& reporter_;         ///< Обработчик ошибок
    const core::symbol_registry& registry_;  ///< Реестр объявленных символов
    core::scoped_map<core::type> symbols_;  ///< Таблица типов переменных в текущей области видимости
    std::optional<core::type> curr_return_type_;  ///< Ожидаемый тип возврата (внутри функции)
};
