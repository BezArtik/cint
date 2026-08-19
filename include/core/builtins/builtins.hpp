/**
 * @file include/core/builtins/builtins.hpp
 * @brief Встроенные функции интерпретатора (standard library).
 * @ingroup CoreUtils
 */

#pragma once
#include "core/type/type.hpp"

#include <span>
#include <string_view>
#include <vector>

namespace core {

class value;

/**
 * @brief Указатель на реализацию builtin-функции.
 *
 * Принимает массив аргументов (уже вычисленных значений) и возвращает результат.
 * Не имеет доступа к интерпретатору — работает только с переданными значениями.
 *
 * @param args Вычисленные аргументы вызова
 * @return Результат функции
 */
using builtin_fn_ptr = value (*)(std::span<const value>);

/**
 * @brief Описание builtin-функции для регистрации в symbol_registry.
 *
 * Используется при построении реестра символов: каждая запись
 * определяет имя, сигнатуру и реализацию встроенной функции.
 */
struct builtin_def {
    std::string_view name_;          ///< Имя функции
    type return_type_;               ///< Тип возвращаемого значения
    std::vector<type> param_types_;  ///< Типы параметров (порядок важен)
    builtin_fn_ptr impl_;            ///< Указатель на реализацию
};

/**
 * @defgroup Builtins Встроенные функции
 * @brief Стандартная библиотека языка.
 * @ingroup CoreUtils
 *
 * Встроенные функции доступны в любой программе без объявления.
 * Пользователь может переопределить builtin, объявив функцию с тем же именем.
 *
 * Категории:
 * - **Ввод/вывод**: print_int, print_dbl, print_bool, print_str, input
 * - **Математика**: sqrt, sin, exp
 * - **Преобразования**: dtoi, stoi, itod, stod
 * - **Генерация случайных чисел**: rand_int, rand_dbl, srand
 */

/// @name Реализации builtin-функций
/// @ingroup Builtins
/// @{

namespace builtin_impl {

/// @name Ввод/вывод
/// @{

value print_int(std::span<const value> args);   ///< Выводит целое число в stdout.
value print_dbl(std::span<const value> args);   ///< Выводит число с плавающей точкой в stdout.
value print_bool(std::span<const value> args);  ///< Выводит булево значение в stdout.
value print_str(std::span<const value> args);   ///< Выводит строку в stdout.
value input(std::span<const value> args);       ///< Читает строку из stdin.

/// @}

/// @name Математические функции
/// @{

value sqrt(std::span<const value> args);  ///< Квадратный корень.
value sin(std::span<const value> args);   ///< Синус (аргумент в радианах).
value exp(std::span<const value> args);   ///< Экспонента e^x.

/// @}

/// @name Преобразования типов
/// @{

value dtoi(std::span<const value> args);  ///< Double → Int (отбрасывание дробной части).
value stoi(std::span<const value> args);  ///< String → Int.
value itod(std::span<const value> args);  ///< Int → Double.
value stod(std::span<const value> args);  ///< String → Double.

/// @}

/// @name Случайные числа
/// @{

/**
 * @brief Случайное целое число в диапазоне [min, max].
 *
 * Использует mt19937_64 с инициализацией от random_device.
 * При каждом запуске последовательность разная (если не задан seed).
 */
value rand_int(std::span<const value> args);

/**
 * @brief Случайное число с плавающей точкой в диапазоне [min, max].
 */
value rand_dbl(std::span<const value> args);

/**
 * @brief Устанавливает seed генератора случайных чисел.
 *
 * Позволяет получить воспроизводимую последовательность.
 */
value srand(std::span<const value> args);

/// @}

}  // namespace builtin_impl

/// @}

/**
 * @brief Реестр всех встроенных функций.
 * @ingroup Builtins
 *
 * Содержит описания (имя, сигнатура, реализация) всех builtin-функций.
 * Используется symbol_registry::build() для начального наполнения реестра.
 *
 * Порядок в реестре определяет только очерёдность регистрации.
 * При совпадении имени с пользовательской функцией приоритет имеет
 * пользовательская (builtin заменяется при обходе AST-объявлений).
 */
inline const std::array builtins = {
    // Ввод/вывод
    builtin_def{"print_int", type::void_type(), {type::int_type()}, builtin_impl::print_int},
    builtin_def{"print_dbl", type::void_type(), {type::double_type()}, builtin_impl::print_dbl},
    builtin_def{"print_bool", type::void_type(), {type::bool_type()}, builtin_impl::print_bool},
    builtin_def{"print_str", type::void_type(), {type::string_type()}, builtin_impl::print_str},

    builtin_def{"input", type::string_type(), {}, builtin_impl::input},

    // Математика
    builtin_def{"sqrt", type::double_type(), {type::double_type()}, builtin_impl::sqrt},
    builtin_def{"sin", type::double_type(), {type::double_type()}, builtin_impl::sin},
    builtin_def{"exp", type::double_type(), {type::double_type()}, builtin_impl::exp},

    // Преобразования
    builtin_def{"dtoi", type::int_type(), {type::double_type()}, builtin_impl::dtoi},
    builtin_def{"stoi", type::int_type(), {type::string_type()}, builtin_impl::stoi},

    builtin_def{"itod", type::double_type(), {type::int_type()}, builtin_impl::itod},
    builtin_def{"stod", type::double_type(), {type::string_type()}, builtin_impl::stod},

    // Случайные числа
    builtin_def{"rand_int", type::int_type(), {type::int_type(), type::int_type()}, builtin_impl::rand_int},
    builtin_def{"rand_dbl", type::double_type(), {type::double_type(), type::double_type()}, builtin_impl::rand_dbl},
    builtin_def{"srand", type::void_type(), {type::int_type()}, builtin_impl::srand}};

}  // namespace core
