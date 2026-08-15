/**
 * @file include/ast/statement.hpp
 * @brief Узлы инструкций и объявлений AST.
 * @ingroup AST
 */

#pragma once

#include "ast/expression.hpp"
#include "core/utils/arena.hpp"

#include <algorithm>
#include <utility>
#include <variant>
#include <vector>

namespace ast {

// forward declaration
struct statement;

/// Список инструкций (тело функции, блок кода).
using stmt_list = std::pmr::vector<node<statement>>;

/**
 * @brief Инструкция-выражение: `expr;`.
 *
 * Вычисляет выражение и отбрасывает результат.
 * Примеры: вызов функции, присваивание.
 */
struct expression_stmt {
    expression expr_;     ///< Выражение для вычисления
    core::location loc_;  ///< Позиция в исходном коде
};

/**
 * @brief Объявление переменной: `type name [= init];`.
 *
 * Может иметь инициализатор. Тип проверяется на этапе type checking.
 * Поддерживает автоматический вывод размера массива из инициализатора.
 */
struct var_declaration {
    core::type type_;                        ///< Тип переменной
    core::token name_;                       ///< Токен имени
    std::optional<expression> initializer_;  ///< Опциональный инициализатор
    core::location loc_;                     ///< Позиция в исходном коде
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
    expression condition_;   ///< Условие продолжения цикла
    node<statement> block_;  ///< Тело цикла
    core::location loc_;     ///< Позиция в исходном коде
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
    node<statement> initializer_;          ///< Инициализатор (может быть nullptr)
    std::optional<expression> condition_;  ///< Условие продолжения
    std::optional<expression> increment_;  ///< Выражение инкремента
    node<statement> block_;                ///< Тело цикла
    core::location loc_;                   ///< Позиция в исходном коде
};

/**
 * @brief Условная инструкция: `if (condition) then [else else_branch]`.
 *
 * Ветка else опциональна. Обе ветки выполняются в своих областях видимости.
 */
struct if_stmt {
    expression condition_;        ///< Условие (должно быть bool)
    node<statement> then_block_;  ///< Ветка then
    node<statement> else_block_;  ///< Ветка else (может быть nullptr)
    core::location loc_;          ///< Позиция в исходном коде
};

/**
 * @brief Инструкция возврата: `return [value];`.
 *
 * Может возвращать значение (в не-void функциях) или ничего (в void-функциях).
 * Проверка соответствия типов выполняется на этапе type checking.
 */
struct return_stmt {
    core::token keyword_;              ///< Токен ключевого слова return
    std::optional<expression> value_;  ///< Возвращаемое значение (опционально)
    core::location loc_;               ///< Позиция в исходном коде
};

/**
 * @brief Параметр функции: `type name`.
 */
struct func_param {
    core::type type_;   ///< Тип параметра
    core::token name_;  ///< Токен имени параметра
};

/**
 * @brief Объявление функции: `return_type name(params) { body }`.
 *
 * Содержит полную сигнатуру и тело. Параметры образуют новую область
 * видимости. Функция может быть вызвана рекурсивно (в пределах MAX_RECURSION_DEPTH).
 */
struct func_declaration {
    core::type return_type_;               ///< Тип возвращаемого значения
    core::token name_;                     ///< Токен имени функции
    std::pmr::vector<func_param> params_;  ///< Список параметров
    node<statement> block_;                ///< Тело функции
    core::location loc_;                   ///< Позиция в исходном коде
};

/**
 * @brief Объявление структуры: `struct name { fields... };`.
 *
 * Определяет новый структурный тип с именованными полями.
 * Тип сохраняется в symbol_registry для дальнейшего использования.
 */
struct struct_declaration {
    core::type type_;     ///< Тип структуры (с полной информацией о полях)
    core::token name_;    ///< Токен имени структуры
    core::location loc_;  ///< Позиция в исходном коде
};

/**
 * @brief Алгебраический тип инструкции AST.
 *
 * Представляет любую инструкцию или объявление как variant.
 * Конструктор шаблонный — принимает любую из структур-вариантов.
 */
struct statement {
    std::variant<expression_stmt, var_declaration, block_stmt, for_stmt, while_stmt, if_stmt, return_stmt,
                 func_declaration, struct_declaration>
        data_;

    statement() = delete;

    template <typename T>
    statement(T s) : data_{std::move(s)} {}
};

/**
 * @brief Создаёт узел инструкции в арене.
 *
 * @tparam Stmt Тип узла инструкции
 * @tparam Args Типы аргументов конструктора (кроме location)
 * @param arena  Арена для размещения
 * @param loc    Позиция в исходном коде
 * @param args   Аргументы конструктора
 * @return Узел node<statement> в AST
 */
template <typename Stmt, typename... Args>
node<statement> make_stmt(core::arena& arena, core::location loc, Args&&... args) {
    return core::make_arena<statement>(arena, Stmt{std::forward<Args>(args)..., loc});
}

/**
 * @brief Проверяет, содержит ли блок объявления переменных.
 *
 * Используется для оптимизации: scope создаётся только если в блоке
 * есть var_declaration. Результат кешируется в has_declarations_.
 *
 * @param stmts Список всех statements
 * @return true, если statement содержит объявления переменных.
 */
inline bool has_declarations(const stmt_list& stmts) noexcept {
    return std::ranges::any_of(stmts, [](auto&& stmt) { return std::holds_alternative<var_declaration>(stmt->data_); });
}

}  // namespace ast
