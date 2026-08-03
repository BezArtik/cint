/**
 * @file include/core/error/error_codes.hpp
 * @brief Коды ошибок и таблица сообщений.
 * @ingroup CoreError
 */

#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace core {
// clang-format off
#define ERROR(X)                                                                                \
    /* Lexer */                                                                                 \
    X(unexpected_character,                  "Unexpected character")                            \
    X(unterminated_string,                   "Unterminated string literal")                     \
    /* Parser */                                                                                \
    X(unexpected_token,                      "Unexpected token")                                \
    X(expected_expression,                   "Expected expression")                             \
    X(expected_semicolon,                    "Expected ';'")                                    \
    X(expected_identifier,                   "Expected identifier")                             \
    X(expected_type,                         "Expected type")                                   \
    X(expected_right_paren,                  "Expected ')'")                                    \
    X(expected_right_brace,                  "Expected '}'")                                    \
    X(expected_left_brace,                   "Expected '{'")                                    \
    X(expected_right_bracket,                "Expected ']'")                                    \
    X(expected_left_paren_while,             "Expected '(' after 'while'")                      \
    X(expected_left_paren_for,               "Expected '(' after 'for'")                        \
    X(expected_left_paren_if,                "Expected '(' after 'if'")                         \
    X(expected_right_paren_condition,        "Expected ')' after condition")                    \
    X(void_variable,                         "Variables cannot be of type void")                \
    /* Type checker */                                                                          \
    X(type_mismatch_initialization,          "Type mismatch in initialization of '{}'")         \
    X(type_mismatch_assignment,              "Type mismatch in assignment")                     \
    X(arithmetic_requires_numeric,           "Arithmetic requires numeric operands")            \
    X(comparison_requires_numeric,           "Comparison requires numeric operands")            \
    X(compound_requires_numeric,             "Compound assignment requires numeric operands")   \
    X(compound_requires_lvalue,              "Compound assignment requires variable")           \
    X(logical_requires_bool,                 "Logical operators require boolean operands")      \
    X(unary_minus_requires_numeric,          "Unary minus requires numeric operand")            \
    X(increment_requires_numeric,            "Increment/decrement requires numeric operand")    \
    X(increment_requires_lvalue,             "Increment/decrement requires variable")           \
    X(not_requires_bool,                     "Logical NOT requires boolean operand")            \
    X(condition_not_bool,                    "{} condition must be a boolean expression")       \
    X(undefined_variable,                    "Undefined variable '{}'")                         \
    X(undefined_function,                    "Undefined function '{}'")                         \
    X(not_a_function,                        "'{}' is not a function")                          \
    X(argument_count_mismatch,               "Function '{}' expects {} arguments, got {}")      \
    X(argument_type_mismatch,                "Argument {} type mismatch in call to '{}'")       \
    X(redeclaration,                         "Redeclaration of '{}'")                           \
    X(return_outside_function,               "Return statement outside of function")            \
    X(return_type_mismatch,                  "Return type mismatch")                            \
    X(return_missing_value,                  "Return with no value in non-void function")       \
    X(unexpected_literal,                    "Unexpected literal type")                         \
    X(empty_initializer_list,                "Initializer list cannot be empty")                \
    X(initializer_list_inconsistent_types,   "Initializer list has inconsistent element types") \
    X(indexing_non_array,                    "Indexing operator applied to non-array type")     \
    X(index_must_be_integer,                 "Array index must be of integer type")             \
    X(not_a_struct,                          "Left side of '.' is not a structure")             \
    X(no_such_field,                         "Structure '{}' has no field '{}'")                \
    X(undefined_type,                        "Undefined type '{}'")                             \
    /* Runtime */                                                                               \
    X(division_by_zero,                      "Division by zero")                                \
    X(modulo_by_zero,                        "Modulo by zero")                                  \
    X(unsupported_binary_operator,           "Unsupported binary operator")                     \
    X(unsupported_unary_operator,            "Unsupported unary operator")                      \
    X(invalid_conversion,                    "Invalid type conversion")                         \
    X(index_out_of_bounds,                   "Index out of bounds")                             \
    X(stack_overflow,                        "Stack overflow")
// clang-format on

/**
 * @brief Коды всех возможных ошибок интерпретатора.
 * @ingroup CoreError
 *
 * Генерируется X-макросом ERROR.
 * Каждый код соответствует одному виду ошибки.
 *
 */
enum class error_code : uint8_t {
#define X(code, msg) code,
    ERROR(X)
#undef X
};

/**
 * @brief Запись таблицы ошибок: код + шаблон сообщения.
 *
 * Шаблон может содержать плейсхолдеры {} для подстановки аргументов.
 * Пример: "Undefined variable '{}'" — {} будет заменён на имя переменной.
 */
struct error_entry {
    error_code code_;          ///< Код ошибки
    std::string_view format_;  ///< Шаблон сообщения (с опциональными {})
};

/**
 * @brief Таблица всех сообщений об ошибках.
 *
 * Индекс в массиве НЕ соответствует значению enum (для этого
 * используйте get_error_message с линейным поиском).
 *
 * Генерируется из ERROR.
 */
inline constexpr std::array error_table = {
#define X(code, msg) error_entry{error_code::code, msg},
    ERROR(X)
#undef X
};

/**
 * @brief Возвращает шаблон сообщения по коду ошибки.
 *
 * Линейный поиск по таблице. Вызывается при каждой ошибке.
 *
 * @param code Код ошибки
 * @return Шаблон сообщения или "Unknown error" для несуществующего кода.
 */
inline std::string_view get_error_message(error_code code) {
    auto it = std::ranges::find(error_table, code, &error_entry::code_);
    return it != error_table.end() ? it->format_ : "Unknown error";
}

/**
 * @brief Исключение: фатальная синтаксическая ошибка.
 *
 * Выбрасывается парсером, когда восстановление невозможно.
 * Перехватывается в parser::parse() для продолжения разбора
 * следующего объявления верхнего уровня.
 */
struct parse_error : std::exception {
    const char* what() const noexcept override { return "Syntax error"; }
};

/**
 * @brief Исключение: ошибка операции над значением.
 *
 * Выбрасывается операциями системы значений (core::ops, value::to_*)
 * при невозможности выполнить операцию: деление на ноль, неверное
 * преобразование типа и т.д.
 *
 * Не зависит от интерпретатора — может возникнуть на любом этапе,
 * где вычисляются значения (лексер при парсинге чисел, builtin-функции).
 */
struct value_error : std::exception {
    error_code code_;

    value_error(error_code c) noexcept : code_(c) {}
    const char* what() const noexcept override { return "Value error"; }
};

/**
 * @brief Исключение: ошибка времени выполнения интерпретатора.
 *
 * Выбрасывается интерпретатором при обходе AST: переполнение стека,
 * выход за границу массива.
 *
 * Отличается от value_error тем, что возникает в контексте выполнения
 * конкретной инструкции/выражения, а не абстрактной операции над значением.
 */
struct runtime_error : std::exception {
    error_code code_;

    runtime_error(error_code c) noexcept : code_(c) {}
    const char* what() const noexcept override { return "Runtime error"; }
};

}  // namespace core
