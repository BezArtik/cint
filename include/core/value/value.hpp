/**
 * @file include/core/value/value.hpp
 * @brief Универсальный тип значения времени выполнения.
 * @ingroup Core
 *
 * @defgroup CoreValue Система значений
 * @brief Типы данных и операции времени выполнения интерпретатора.
 */

#pragma once
#include "core/type/type.hpp"
#include "core/utils/variant.hpp"

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
    using int_t = int64_t;               ///< Целочисленный тип (64 бита)
    using double_t = double;             ///< Тип с плавающей точкой
    using bool_t = bool;                 ///< Булев тип
    using string_t = std::string;        ///< Строка (пользовательский тип)
    using array_t = std::vector<value>;  ///< Массив значений (пользовательский тип)
    using struct_t = struct_data;        ///< Структура (значение + тип)

    /// Создает void-значение
    value() : data_{std::monostate{}} {}

    /// Создают примитивные типы.
    value(int_t v) : data_{std::move(v)} {}
    value(double_t v) : data_{std::move(v)} {}
    value(bool_t v) : data_{std::move(v)} {}

    /// Создаёт строковое значение.
    value(string_t v) : data_{std::make_shared<string_t>(std::move(v))} {}

    /// Создаёт массив.
    value(array_t v) : data_{std::make_shared<array_t>(std::move(v))} {}

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
     * @brief Парсит строковое представление числа.
     *
     *
     * @param text      Строковое представление числа
     * @param is_double true — парсить как double, false — как int
     * @return Распарсенное значение.
     * @throws core::value_error, если строка не является числом.
     */
    static value from_string(std::string_view text, bool is_double);

    /// @name Преобразования к примитивным типам
    /// @{

    /**
     * @brief Преобразует значение к value::int_t.
     * @throws core::value_error, если преобразование невозможно.
     */
    int_t to_int() const;

    /**
     * @brief Преобразует значение к value::double_t.
     * @throws core::value_error, если преобразование невозможно.
     */
    double_t to_double() const;

    /**
     * @brief Преобразует значение к value::bool_t.
     * @throws core::value_error, если преобразование невозможно.
     */
    bool_t to_bool() const;

    /**
     * @brief Преобразует значение к value::string_t.
     * @throws core::value_error, если преобразование невозможно.
     */
    string_t to_string() const;

    /// @}

    /// @name Доступ к составным типам
    /// @{

    /**
     * @brief Получить ссылку на массив.
     *
     * @return Ссылка на вектор значений.
     * @throws core::value_error, если значение не является массивом.
     */
    const array_t& to_array() const;
    array_t& to_array();

    /**
     * @brief Получить ссылку на структуру.
     *
     * @return Ссылка на данные структуры.
     * @throws core::value_error, если значение не является структурой.
     */
    const struct_t& to_struct() const;
    struct_t& to_struct();

    /// @}

    /// @name Проверка на существование значения.
    /// @{

    bool is_int() const noexcept;
    bool is_double() const noexcept;
    bool is_bool() const noexcept;
    bool is_string() const noexcept;
    bool is_array() const noexcept;
    bool is_struct() const noexcept;
    bool is_void() const noexcept;

    /// @}

private:
    /**
     * @brief Данные структурного типа.
     *
     * Хранит тип структуры (для доступа к полям по имени) и вектор значений полей.
     */
    struct struct_data {
        core::type type_;            ///< Тип структуры (с информацией о полях)
        std::vector<value> fields_;  ///< Значения полей (в порядке объявления)
    };

    using string_wrap = std::shared_ptr<string_t>;
    using array_wrap = std::shared_ptr<array_t>;

    core::variant<int_t, double_t, bool_t, string_wrap, array_wrap, struct_t, std::monostate> data_;
};

}  // namespace core
