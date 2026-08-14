/**
 * @file include/core/token/type.hpp
 * @brief Система статических типов языка.
 * @ingroup Core
 *
 * @defgroup CoreType Система типов
 * @brief Представление и проверка типов на этапе компиляции.
 */

#pragma once
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

namespace core {

/**
 * @brief Статический тип в языке.
 * @ingroup CoreType
 *
 * Представляет тип, известный на этапе компиляции (до выполнения).
 * Поддерживает:
 * - **Примитивные типы**: int, double, bool, string, void
 * - **Составные типы**: функция, массив, структура
 * - **Служебные**: unknown (для ошибок вывода типов)
 *
 * Реализация:
 * - Примитивные типы — легковесные, без динамической памяти
 * - Функции и массивы — рекурсивно владеют вложенными типами через unique_ptr
 * - Структуры — хранят имя и список полей (имя + тип)
 *
 * Идентичность типов:
 * - Примитивные: сравниваются по kind
 * - Функции: сравниваются по возвращаемому типу и типам параметров
 * - Массивы: сравниваются по размеру и типу элементов в списке инициализации
 * - Структуры: сравниваются по имени (номинативная типизация)
 *
 */
class type {
public:

    /// @brief Поле структуры: имя + тип.
    using field_t = std::pair<std::string_view, type>;

    type() = default;

    /// @name Фабрики примитивных типов
    /// @{

    static type int_type();
    static type double_type();
    static type bool_type();
    static type string_type();
    static type void_type();
    static type unknown_type();

    /// @}

    /// @name Фабрики составных типов
    /// @{

    /**
     * @brief Создаёт функциональный тип.
     *
     * @param return_type Тип возвращаемого значения
     * @param param_types Типы параметров (порядок важен)
     * @return Функциональный тип.
     */
    static type function_type(type return_type, std::vector<type> param_types);

    /**
     * @brief Создаёт тип массива.
     *
     * @param element_type Тип элементов
     * @param size         Размер массива (0 — неизвестный размер)
     * @return Тип массива.
     */
    static type array_type(type element_type, size_t size = 0);

    /**
     * @brief Создаёт структурный тип.
     *
     * @param name   Имя структуры (для номинативной типизации)
     * @param fields Список полей (имя + тип)
     * @return Структурный тип.
     */
    static type struct_type(std::string_view name, std::vector<field_t> fields);

    /// @}

    /// @name Проверка категории
    /// @{

    /// Числовой тип (int или double).
    bool is_numeric() const noexcept;

    bool is_int() const noexcept;
    bool is_double() const noexcept;
    bool is_bool() const noexcept;
    bool is_string() const noexcept;
    bool is_void() const noexcept;
    bool is_unknown() const noexcept;
    bool is_function() const noexcept;
    bool is_array() const noexcept;
    bool is_struct() const noexcept;

    /// @}

    /// @name Доступ к параметрам составных типов
    /// @{

    /// Возвращаемый тип функции. @pre is_function()
    const type& return_type() const;

    /// Типы параметров функции. @pre is_function()
    std::span<const type> param_types() const;

    /// Тип элемента массива. @pre is_array()
    const type& element_type() const;

    /// Размер массива. @pre is_array()
    size_t array_size() const;

    /// Имя структуры. @pre is_struct()
    std::string_view struct_name() const;

    /// Поля структуры. @pre is_struct()
    std::span<const field_t> struct_fields() const;

    /**
     * @brief Поиск индекса поля по имени.
     *
     * @param name Имя поля
     * @return Индекс поля или std::nullopt, если поле не найдено.
     * @pre is_struct()
     */
    std::optional<size_t> field_index(std::string_view name) const noexcept;

    /// @}

    /// @name Сравнение
    /// @{

    /**
     * @brief Сравнение типов.
     *
     * - Примитивные: по kind
     * - Функции: по возвращаемому типу и параметрам
     * - Массивы: по размеру и типу элементов
     * - Структуры: по имени
     */
    bool operator==(const type& other) const noexcept;
    bool operator!=(const type& other) const noexcept;

    /// @}

private:
    /// @cond INTERNAL

    struct int_t {};
    struct double_t {};
    struct bool_t {};
    struct string_t {};
    struct void_t {};
    struct unknown_t {};
    struct function_t {
        std::unique_ptr<type> return_type_;
        std::vector<type> param_types_;
    };
    struct array_t {
        std::unique_ptr<type> element_type_;
        size_t size_;
    };
    struct struct_t {
        std::string_view name_;
        std::vector<field_t> fields_;
    };

    template <typename Info>
    type(Info info) : data_(std::move(info)) {}

    std::variant<
        int_t,
        double_t,
        bool_t,
        string_t,
        void_t,
        std::shared_ptr<function_t>,
        std::shared_ptr<array_t>,
        std::shared_ptr<struct_t>,
        unknown_t
    > data_;

    /// @endcond
};

}  // namespace core
