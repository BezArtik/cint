/**
 * @file include/core/value/value.hpp
 * @brief Универсальный тип значения времени выполнения.
 * @ingroup Core
 *
 * @defgroup CoreValue Система значений
 * @brief Типы данных и операции времени выполнения интерпретатора.
 */

#pragma once
#include "core/token/type.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace core {

/**
 * @brief Универсальное значение времени выполнения.
 * @ingroup CoreValue
 *
 * Представляет любое значение, с которым работает интерпретатор:
 * числа, строки, булевы значения, массивы, структуры, void.
 *
 * Реализовано как **type-erased обёртка** над std::variant. 
 *
 * @invariant Значение всегда содержит один из допустимых альтернативных типов.
 *            Конструктор по умолчанию создаёт void-значение (monostate).
 *
 */
class value {
    /// @cond INTERNAL
    struct struct_data;
    /// @endcond

public:
    using int_t = int64_t;                                ///< Целочисленный тип (64 бита)
    using double_t = double;                              ///< Тип с плавающей точкой
    using bool_t = bool;                                  ///< Булев тип
    using string_t = std::shared_ptr<std::string>;        ///< Строка (разделяемая)
    using array_t = std::shared_ptr<std::vector<value>>;  ///< Массив значений (разделяемый)
    using struct_t = struct_data;                         ///< Структура (значение + тип)

    /// Создает void-значение
    value() : data_{std::monostate{}} {}

    /// Создают примитивные типы.
    value(int_t v) : data_{v} {}
    value(double_t v) : data_{v} {}
    value(bool_t v) : data_{v} {}

    /// Создаёт строковое значение (размещает строку в shared_ptr).
    value(std::string v) : data_{std::make_shared<std::string>(std::move(v))} {}

    /// Создаёт массив (размещает вектор в shared_ptr).
    value(std::vector<value> v) : data_{std::make_shared<std::vector<value>>(std::move(v))} {}

    /// Создает структуру
    value(struct_t v) : data_{std::move(v)} {}

    /**
     * @brief Возвращает значение по умолчанию для указанного типа.
     *
     * - Числа: 0
     * - Bool: false
     * - Строка: ""
     * - Массив: массив заданного размера с default-элементами
     * - Структура: структура с default-полями
     * - Void: monostate
     *
     * @param t Тип, для которого нужно значение по умолчанию.
     * @return Инициализированное значение.
     */
    static value default_value(const core::type& t);

    /**
     * @brief Преобразует значение к целевому типу.
     *
     * Поддерживаемые преобразования:
     * - int → double
     * - Тождественное (если типы уже совпадают)
     *
     * @param val    Исходное значение
     * @param target Целевой тип
     * @return Преобразованное значение.
     */
    static value convert(core::value val, const core::type& target);

    /**
     * @brief Парсит строковое представление числа.
     *
     * Использует std::from_chars для эффективного парсинга без аллокаций.
     *
     * @param text      Строковое представление числа
     * @param is_double true — парсить как double, false — как int
     * @return Распарсенное значение.
     * @throws core::interpret_error если строка не является числом.
     */
    static value from_string(std::string_view text, bool is_double);

    /**
     * @brief Преобразует значение к int.
     *
     * Правила:
     * - int → значение как есть
     * - double → отбрасывание дробной части
     * - string → парсинг как int
     * - остальное → interpret_error
     */
    int_t to_int() const;

    /**
     * @brief Преобразует значение к double.
     *
     * Правила:
     * - int → повышение до double
     * - double → значение как есть
     * - string → парсинг как double
     * - остальное → interpret_error
     */
    double_t to_double() const;

    /**
     * @brief Преобразует значение к bool.
     *
     * Правила:
     * - int/double: != 0 → true
     * - bool: значение как есть
     * - string: непустая → true
     * - array: непустой → true
     * - остальное → interpret_error
     */
    bool_t to_bool() const;

    /// Преобразует значение в строковое представление.
    std::string to_string() const;

    /// Возвращает тип значения как core::type.
    core::type type() const;

    bool is_int() const noexcept;
    bool is_double() const noexcept;
    bool is_bool() const noexcept;
    bool is_string() const noexcept;
    bool is_array() const noexcept;
    bool is_struct() const noexcept;
    bool is_void() const noexcept;

    /**
     * @brief Получить значение конкретного типа (константный доступ).
     *
     * @tparam T Ожидаемый тип (int_t, double_t, string_t, array_t, struct_t)
     * @return Указатель на значение или nullptr, если тип не совпадает.
     */
    template <typename T>
    const T* as() const noexcept {
        return std::get_if<T>(&data_);
    }

    /**
     * @brief Получить значение конкретного типа (изменяемый доступ).
     *
     * Используется для модификации значений на месте (инкремент, присваивание в массив).
     *
     * @tparam T Ожидаемый тип
     * @return Указатель на значение или nullptr, если тип не совпадает.
     */
    template <typename T>
    T* as_mut() noexcept {
        return std::get_if<T>(&data_);
    }

private:
    /// @cond INTERNAL

    static int_t parse_int(std::string_view text);
    static double_t parse_double(std::string_view text);

    /**
     * @brief Данные структурного типа.
     *
     * Хранит тип структуры (для доступа к полям по имени) и вектор значений полей.
     */
    struct struct_data {
        core::type type_;            ///< Тип структуры (с информацией о полях)
        std::vector<value> fields_;  ///< Значения полей (в порядке объявления)
    };

    std::variant<int_t, double_t, bool_t, string_t, array_t, struct_t, std::monostate> data_;

    /// @endcond
};

}  // namespace core
