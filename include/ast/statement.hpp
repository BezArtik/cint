/**
 * @file include/ast/statement.hpp
 * @brief Узлы инструкций и объявлений AST.
 * @ingroup AST
 */

#pragma once

#include "ast/node.hpp"
#include "core/memory/arena.hpp"
#include "core/token/token.hpp"
#include "core/type/type.hpp"

#include <optional>
#include <utility>

namespace ast {

/**
 * @brief Инструкция-выражение: `expr;`.
 *
 * Вычисляет выражение и отбрасывает результат.
 * Примеры: вызов функции, присваивание.
 */
struct expression_stmt {
    expression expr_;  ///< Выражение для вычисления
};

/**
 * @brief Объявление переменной: `type name [= init];`.
 *
 * Может иметь инициализатор. Тип проверяется на этапе type checking.
 * Поддерживает автоматический вывод размера массива из инициализатора.
 */
struct var_declaration_stmt {
    core::type type_;                        ///< Тип переменной
    core::token name_;                       ///< Токен имени
    std::optional<expression> initializer_;  ///< Инициализатор
};

/**
 * @brief Блок инструкций: `{ statements... }`.
 *
 * Создаёт новую область видимости для переменных, объявленных внутри.
 * Флаг has_declarations используется для оптимизации: если в блоке
 * нет объявлений, scope при выполнении не создаётся.
 */
struct block_stmt {
    stmt_list statements_;   ///< Список инструкций блока
    bool has_declarations_;  ///< Флаг для оптимизации
    core::location loc_;     ///< Позиция в исходном коде
};

/**
 * @brief Цикл while: `while (condition) body`.
 *
 * Условие вычисляется на каждой итерации. Если ложно — цикл завершается.
 * Тело может быть любой инструкцией, не только блоком.
 */
struct while_stmt {
    expression condition_;  ///< Условие продолжения цикла
    statement block_;       ///< Тело цикла
    core::location loc_;    ///< Позиция в исходном коде
};

/**
 * @brief Цикл for: `for (init; condition; increment) body`.
 *
 * Все три части опциональны. Порядок выполнения:
 * 1. init (один раз)
 * 2. condition (если есть и false — выход)
 * 3. body
 * 4. increment
 * 5. переход к п.2
 */
struct for_stmt {
    std::optional<statement> initializer_;  ///< Инициализатор
    std::optional<expression> condition_;   ///< Условие продолжения
    std::optional<expression> increment_;   ///< Выражение инкремента
    statement block_;                       ///< Тело цикла
    core::location loc_;                    ///< Позиция в исходном коде
};

/**
 * @brief Условная инструкция: `if (condition) then [else else_branch]`.
 *
 * Ветка else опциональна. Обе ветки выполняются в своих областях видимости.
 */
struct if_stmt {
    expression condition_;                 ///< Условие (должно быть bool)
    statement then_block_;                 ///< Ветка then
    std::optional<statement> else_block_;  ///< Ветка else
    core::location loc_;                   ///< Позиция в исходном коде
};

/**
 * @brief Инструкция возврата: `return [value];`.
 *
 * Может возвращать значение (в не-void функциях) или ничего (в void-функциях).
 * Проверка соответствия типов выполняется на этапе type checking.
 */
struct return_stmt {
    std::optional<expression> value_;  ///< Возвращаемое значение
    core::location loc_;               ///< Позиция в исходном коде
};

/**
 * @brief Объявление функции: `return_type name(params) { body }`.
 *
 * Содержит полную сигнатуру в типе и тело. Параметры образуют новую область
 * видимости. Функция может быть вызвана рекурсивно (в пределах MAX_RECURSION_DEPTH).
 */
struct func_declaration_stmt {
    core::type type_;     ///< Тип функции (имя, тип возврата, информация о параметрах)
    statement block_;     ///< Тело функции
    core::location loc_;  ///< Позиция в исходном коде
};

/**
 * @brief Объявление структуры: `struct name { fields... };`.
 *
 * Определяет новый структурный тип с именованными полями.
 * Тип сохраняется в symbol_registry для дальнейшего использования.
 */
struct struct_declaration_stmt {
    core::type type_;     ///< Тип структуры (с полной информацией о полях)
    core::location loc_;  ///< Позиция в исходном коде
};

/**
 * @brief Создаёт узел инструкции в арене.
 *
 * Размещает объект типа Stmt в переданной арене и возвращает
 * statement, содержащий node<Stmt>.
 *
 * @tparam Stmt Тип узла инструкции
 * @tparam Args Типы аргументов конструктора
 * @param arena  Арена для размещения
 * @param args   Аргументы конструктора
 * @return Готовый statement с node<Stmt>.
 */
template <typename Stmt, typename... Args>
statement make_stmt(core::arena& arena, Args&&... args) {
    return statement{core::make_arena<Stmt>(arena, std::forward<Args>(args)...)};
}

}  // namespace ast
