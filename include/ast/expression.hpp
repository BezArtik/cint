/**
 * @file include/ast/expression.hpp
 * @brief Узлы выражений AST.
 * @ingroup AST
 *
 * @defgroup AST Абстрактное синтаксическое дерево
 * @brief Типы данных, представляющие узлы AST — выражения, инструкции, объявления.
 */

#pragma once

#include "core/token/token.hpp"
#include "core/utils/arena.hpp"

#include <variant>
#include <vector>

namespace ast {

// Forward declarations
struct literal_expr;
struct variable_expr;
struct binary_expr;
struct assignment_expr;
struct unary_expr;
struct postfix_expr;
struct call_expr;
struct initializer_list_expr;
struct index_expr;
struct member_access_expr;

/**
 * @brief Шаблонный алиас для узла AST
 *
 * Тип, которым представлены все узлы AST.
 *
 */
template <typename T>
using node = core::arena_ptr<T>;

/**
 * @brief Алгебраический тип выражения AST.
 *
 * Представляет любое выражение как variant всех возможных
 * видов выражений. Указатели на сложные узлы (binary_expr, call_expr, ...)
 * обёрнуты в core::arena_ptr — время жизни узлов управляется ареной.
 *
 * @see make_expr(), make_expr_val()
 */
// clang-format off
using expression =
    std::variant<node<literal_expr>, node<variable_expr>, 
                 node<binary_expr>,  node<assignment_expr>,
                 node<unary_expr>,   node<postfix_expr>, 
                 node<call_expr>,    node<initializer_list_expr>, 
                 node<index_expr>,   node<member_access_expr>>;
// clang-format on

/// Список выражений (аргументы вызова, элементы инициализатора).
using expr_list = std::pmr::vector<expression>;

/**
 * @brief Литеральное значение: число, строка, true/false.
 *
 * Содержит токен с уже вычисленным литеральным значением
 * (core::token::literal_value_).
 */
struct literal_expr {
    core::token value_;   ///< Токен с литеральным значением
    core::location loc_;  ///< Позиция в исходном коде

    literal_expr(const core::token& value, core::location loc) : value_{value}, loc_{loc} {}
};

/**
 * @brief Обращение к переменной по имени.
 *
 * Самая простая форма lvalue-выражения. Семантика: чтение значения
 * переменной с указанным именем.
 */
struct variable_expr {
    core::token name_;    ///< Токен имени переменной
    core::location loc_;  ///< Позиция в исходном коде

    variable_expr(const core::token& name, core::location loc) : name_{name}, loc_{loc} {}
};

/**
 * @brief Бинарная операция: `a + b`, `x < y`, `a && b`.
 *
 * Поддерживает арифметические, сравнения, логические и битовые операторы.
 * Порядок вычисления операндов: слева направо (кроме && и || с коротким замыканием).
 */
struct binary_expr {
    expression left_;     ///< Левый операнд
    core::token op_;      ///< Токен оператора
    expression right_;    ///< Правый операнд
    core::location loc_;  ///< Позиция в исходном коде

    binary_expr(expression left, const core::token& op, expression right, core::location loc)
        : left_{std::move(left)}, op_{op}, right_{std::move(right)}, loc_{loc} {}
};

/**
 * @brief Присваивание: `a = b`, `x += 1`, `arr[0] = 5`.
 *
 * Поддерживает как простое присваивание, так и составные операторы
 * (+=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=).
 * Левая часть должна быть lvalue-выражением.
 */
struct assignment_expr {
    expression target_;   ///< Lvalue-цель присваивания
    core::token op_;      ///< Токен оператора присваивания
    expression value_;    ///< Присваиваемое значение
    core::location loc_;  ///< Позиция в исходном коде

    assignment_expr(expression target, const core::token& op, expression value, core::location loc)
        : target_{std::move(target)}, op_{op}, value_{std::move(value)}, loc_{loc} {}
};

/**
 * @brief Унарная операция: `-x`, `!flag`, `++i`, `--i`.
 *
 * Все унарные операторы префиксные. Постфиксные варианты
 * представлены отдельным узлом postfix_expr.
 */
struct unary_expr {
    core::token op_;      ///< Токен унарного оператора
    expression operand_;  ///< Операнд
    core::location loc_;  ///< Позиция в исходном коде

    unary_expr(const core::token& op, expression operand, core::location loc)
        : op_{op}, operand_{std::move(operand)}, loc_{loc} {}
};

/**
 * @brief Постфиксная операция: `i++`, `i--`.
 *
 * Отличается от префиксной семантикой: возвращает старое значение
 * до изменения. Операнд должен быть lvalue.
 */
struct postfix_expr {
    expression operand_;  ///< Операнд (должен быть lvalue)
    core::token op_;      ///< Токен оператора (INCREMENT или DECREMENT)
    core::location loc_;  ///< Позиция в исходном коде

    postfix_expr(expression operand, core::token op, core::location loc)
        : operand_{std::move(operand)}, op_{op}, loc_{loc} {}
};

/**
 * @brief Вызов функции: `callee(args)`.
 *
 * Содержит токен имени функции и список аргументов.
 * Разрешение имени (пользовательская функция или builtin)
 * выполняется на этапе семантического анализа.
 */
struct call_expr {
    core::token callee_;  ///< Токен имени функции
    expr_list args_;      ///< Список аргументов
    core::location loc_;  ///< Позиция в исходном коде

    call_expr(const core::token& callee, expr_list args, core::location loc)
        : callee_{callee}, args_{std::move(args)}, loc_{loc} {}
};

/**
 * @brief Литерал массива: `{1, 2, 3}`.
 *
 * Используется для инициализации массивов. Все элементы должны
 * иметь совместимые типы (проверяется на этапе type checking).
 */
struct initializer_list_expr {
    expr_list elements_;  ///< Список элементов
    core::location loc_;  ///< Позиция в исходном коде

    initializer_list_expr(expr_list elements, core::location loc) : elements_{std::move(elements)}, loc_{loc} {}
};

/**
 * @brief Индексация массива: `object[index]`.
 *
 * Объект должен быть массивом, индекс — целым числом.
 * Допустимо многомерное индексирование: `matrix[i][j]`.
 */
struct index_expr {
    expression object_;   ///< Выражение-массив
    expression index_;    ///< Индексное выражение
    core::location loc_;  ///< Позиция в исходном коде

    index_expr(expression object, expression index, core::location loc)
        : object_{std::move(object)}, index_{std::move(index)}, loc_{loc} {}
};

/**
 * @brief Доступ к полю структуры: `obj.field`.
 *
 * Левая часть должна быть выражением структурного типа.
 * Конкретное поле разрешается на этапе type checking.
 */
struct member_access_expr {
    expression object_;   ///< Выражение-структура
    core::token member_;  ///< Токен имени поля
    core::location loc_;  ///< Позиция в исходном коде

    member_access_expr(expression object, const core::token& member, core::location loc)
        : object_{std::move(object)}, member_{member}, loc_{loc} {}
};

/**
 * @brief Создаёт узел сложного выражения в арене.
 *
 * Размещает объект типа T в переданной арене и возвращает
 * expression, содержащий arena_ptr<T>.
 *
 * @tparam T     Тип узла (binary_expr, call_expr, ...)
 * @tparam Args  Типы аргументов конструктора T
 * @param arena  Арена для размещения
 * @param loc    Позиция в исходном коде
 * @param args   Аргументы конструктора (кроме location)
 * @return Готовый expression с arena_ptr<T>.
 *
 * @note location всегда передаётся последним аргументом в конструктор T.
 */
template <typename T, typename... Args>
expression make_expr(core::arena& arena, core::location loc, Args&&... args) {
    return expression{core::make_arena<T>(arena, std::forward<Args>(args)..., loc)};
}

}  // namespace ast
