/**
 * @file include/ast/statement.hpp
 * @brief Узлы инструкций и объявлений AST.
 * @ingroup AST
 */

#pragma once

#include "ast/expression.hpp"
#include "core/utils/arena.hpp"

#include <algorithm>
#include <variant>
#include <vector>

namespace ast {

// forward declaration
struct statement;

/// Указатель на узел инструкции, размещённый в арене.
using stmt_ptr = core::arena_ptr<statement>;

/// Список инструкций (тело функции, блок кода).
using stmt_list = std::pmr::vector<stmt_ptr>;

/**
 * @brief Инструкция-выражение: `expr;`.
 *
 * Вычисляет выражение и отбрасывает результат.
 * Примеры: вызов функции, присваивание.
 */
struct expression_stmt {
    expression expr_;     ///< Выражение для вычисления
    core::location loc_;  ///< Позиция в исходном коде

    expression_stmt(expression e, core::location loc) : expr_(std::move(e)), loc_(loc) {}
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

    var_declaration(core::type t, const core::token& n, std::optional<expression> init, core::location loc)
        : type_(std::move(t)), name_(n), initializer_(std::move(init)), loc_(loc) {}
};

/**
 * @brief Блок инструкций: `{ statements... }`.
 *
 * Создаёт новую область видимости для переменных, объявленных внутри.
 * Флаг has_declarations используется для оптимизации: если в блоке
 * нет объявлений, scope при выполнении не создаётся.
 */
struct block_stmt {
    stmt_list statements_;                           ///< Список инструкций блока
    core::location loc_;                             ///< Позиция в исходном коде
    mutable bool has_declarations_checked_ = false;  ///< Была ли проверка на объявления
    mutable bool has_declarations_ = false;  ///< Содержит ли блок объявления переменных

    block_stmt() = default;
    block_stmt(stmt_list statements, core::location loc) : statements_(std::move(statements)), loc_(loc) {}
};

/**
 * @brief Цикл while: `while (condition) body`.
 *
 * Условие вычисляется на каждой итерации. Если ложно — цикл завершается.
 * Тело может быть любой инструкцией, не только блоком.
 */
struct while_stmt {
    expression condition_;  ///< Условие продолжения цикла
    stmt_ptr block_;        ///< Тело цикла
    core::location loc_;    ///< Позиция в исходном коде

    while_stmt(expression cond, stmt_ptr block, core::location loc)
        : condition_(std::move(cond)), block_(std::move(block)), loc_(loc) {}
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
    stmt_ptr initializer_;                 ///< Инициализатор (может быть nullptr)
    std::optional<expression> condition_;  ///< Условие продолжения
    std::optional<expression> increment_;  ///< Выражение инкремента
    stmt_ptr block_;                       ///< Тело цикла
    core::location loc_;                   ///< Позиция в исходном коде

    for_stmt(stmt_ptr init, std::optional<expression> cond, std::optional<expression> inc, stmt_ptr block,
             core::location loc)
        : initializer_(std::move(init)),
          condition_(std::move(cond)),
          increment_(std::move(inc)),
          block_(std::move(block)),
          loc_(loc) {}
};

/**
 * @brief Условная инструкция: `if (condition) then [else else_branch]`.
 *
 * Ветка else опциональна. Обе ветки выполняются в своих областях видимости.
 */
struct if_stmt {
    expression condition_;  ///< Условие (должно быть bool)
    stmt_ptr then_block_;   ///< Ветка then
    stmt_ptr else_block_;   ///< Ветка else (может быть nullptr)
    core::location loc_;    ///< Позиция в исходном коде

    if_stmt(expression cond, stmt_ptr then_block, stmt_ptr else_block, core::location loc)
        : condition_(std::move(cond)),
          then_block_(std::move(then_block)),
          else_block_(std::move(else_block)),
          loc_(loc) {}
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

    return_stmt(const core::token& kw, std::optional<expression> val, core::location loc)
        : keyword_(kw), value_(std::move(val)), loc_(loc) {}
};

/**
 * @brief Параметр функции: `type name`.
 */
struct func_param {
    core::type type_;   ///< Тип параметра
    core::token name_;  ///< Токен имени параметра

    func_param(core::type t, const core::token& n) : type_(std::move(t)), name_(n) {}
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
    core::arena_ptr<block_stmt> block_;    ///< Тело функции
    core::location loc_;                   ///< Позиция в исходном коде

    func_declaration(core::type ret_type, const core::token& n)
        : return_type_(std::move(ret_type)), name_(n), loc_(n.loc_) {}
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

    struct_declaration(core::type type, const core::token& name, core::location loc)
        : type_(std::move(type)), name_(name), loc_(loc) {}
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
    statement(T s) : data_(std::move(s)) {}
};

/**
 * @brief Создаёт узел инструкции в арене.
 *
 * @tparam Stmt Тип узла инструкции
 * @tparam Args Типы аргументов конструктора (кроме location)
 * @param arena  Арена для размещения
 * @param loc    Позиция в исходном коде
 * @param args   Аргументы конструктора
 * @return Указатель stmt_ptr на размещённый узел.
 */
template <typename Stmt, typename... Args>
stmt_ptr make_stmt(core::arena& arena, core::location loc, Args&&... args) {
    auto* p = arena.allocate<statement>(Stmt{std::forward<Args>(args)..., loc});
    return stmt_ptr(p);
}

/**
 * @brief Создаёт узел инструкции в арене (принимает готовую структуру).
 *
 * Используется, когда инструкция уже сконструирована (например, func_declaration).
 *
 * @tparam Stmt Тип узла инструкции
 * @param arena  Арена для размещения
 * @param stmt   Готовая структура инструкции
 * @return Указатель stmt_ptr на размещённый узел.
 */
template <typename Stmt>
stmt_ptr make_stmt(core::arena& arena, Stmt&& stmt) {
    auto* p = arena.allocate<statement>(std::forward<Stmt>(stmt));
    return stmt_ptr(p);
}

/**
 * @brief Проверяет, содержит ли блок объявления переменных.
 *
 * Используется для оптимизации: scope создаётся только если в блоке
 * есть var_declaration. Результат кешируется в has_declarations_.
 *
 * @param block Блок инструкций
 * @return true, если блок (рекурсивно) содержит объявления переменных.
 */
inline bool has_declarations(const block_stmt& block) noexcept {
    if (!block.has_declarations_checked_) {
        block.has_declarations_ = std::ranges::any_of(block.statements_, [](const auto& stmt) {
            return std::holds_alternative<var_declaration>(stmt->data_) ||
                   (std::holds_alternative<block_stmt>(stmt->data_) &&
                    has_declarations(std::get<block_stmt>(stmt->data_)));
        });
        block.has_declarations_checked_ = true;
    }
    return block.has_declarations_;
}

}  // namespace ast
