/**
 * @file include/core/utils/variant_wrapper.hpp
 * @brief Универсальная обёртка над std::variant с проверкой полноты.
 * @ingroup CoreUtils
 */

#pragma once

#include <concepts>
#include <type_traits>
#include <utility>
#include <variant>

namespace core {

namespace detail {

/// Вспомогательный шаблон для извлечения типов из std::variant.
template <typename T>
struct variant_types;

template <typename... Ts>
struct variant_types<std::variant<Ts...>> {
    static consteval std::type_identity<std::variant<Ts...>> check() { return {}; }
};

/**
 * @brief Концепт: visitor обрабатывает все типы variant'а.
 * @ingroup CoreVisit
 *
 * Проверяет на этапе компиляции, что visitor имеет перегрузки
 * для каждого типа из variant. Защищает от пропущенных типов
 * при модификации variant в будущем.
 *
 * @tparam Visitor Тип visitor'а (обычно overloaded<...>)
 * @tparam Variant Тип variant'а
 */
template <typename Visitor, typename Variant>
concept exhaustive_visitor = []<typename... Types>(std::type_identity<std::variant<Types...>>) {
    return (std::invocable<Visitor, Types> && ...);
}(variant_types<std::remove_cvref_t<Variant>>::check());

/**
 * @brief Концепт для проверки, входит ли тип в список.
 * @tparam T Проверяемый тип
 * @tparam Types Список допустимых типов
 */
template <typename T, typename... Types>
concept one_of = (std::same_as<T, Types> || ...);

}  // namespace detail

/**
 * @brief Универсальная обёртка над std::variant.
 * @ingroup CoreUtils
 *
 * Предоставляет единый интерфейс для работы с variant:
 * - **Проверка типа**: holds<T>()
 * - **Доступ**: get<T>(), get_if<T>()
 * - **Посещение**: visit() с проверкой полноты
 *
 * @tparam Types Типы, которые могут храниться в variant
 *
 * @invariant Всегда содержит один из допустимых типов.
 */
template <typename... Types>
class variant {
    static_assert(sizeof...(Types) > 0, "variant must have at least one type");

public:
    /// @name Конструкторы
    /// @{

    /**
     * @brief Конструктор по умолчанию.
     * Создаёт variant с первым типом (если он default-constructible).
     */
    variant() = default;

    /**
     * @brief Конструирует обёртку из значения.
     * @tparam T Тип значения (должен быть одним из Types...)
     * @param value Значение для хранения
     */
    template <typename T>
        requires detail::one_of<T, Types...>
    variant(T&& value) : data_{std::forward<T>(value)} {}

    /// @}

    /// @name Проверка типа
    /// @{

    /**
     * @brief Проверяет, хранит ли обёртка значение указанного типа.
     * @tparam T Проверяемый тип (должен быть одним из Types...)
     * @return true, если обёртка хранит T
     */
    template <typename T>
        requires detail::one_of<T, Types...>
    bool holds() const noexcept {
        return std::holds_alternative<T>(data_);
    }

    /// @}

    /// @name Доступ к значению
    /// @{

    /**
     * @brief Возвращает ссылку на хранимое значение.
     * @tparam T Тип значения (должен быть одним из Types...)
     * @pre holds<T>() == true
     * @return Ссылка на значение
     * @throws std::bad_variant_access если тип не совпадает
     */
    template <typename T>
        requires detail::one_of<T, Types...>
    T& get() {
        return std::get<T>(data_);
    }

    /**
     * @brief Константная версия get().
     */
    template <typename T>
        requires detail::one_of<T, Types...>
    const T& get() const {
        return std::get<T>(data_);
    }

    /**
     * @brief Возвращает указатель на хранимое значение.
     * @tparam T Тип значения (должен быть одним из Types...)
     * @return Указатель на значение или nullptr, если тип не совпадает
     */
    template <typename T>
        requires detail::one_of<T, Types...>
    T* get_if() noexcept {
        return std::get_if<T>(&data_);
    }

    /**
     * @brief Константная версия get_if().
     */
    template <typename T>
        requires detail::one_of<T, Types...>
    const T* get_if() const noexcept {
        return std::get_if<T>(&data_);
    }

    /// @}

    /// @name Посещение
    /// @{

    /**
     * @brief Применяет visitor к хранимому значению.
     *
     * Visitor должен быть exhaustive — иметь перегрузки operator()
     * для всех типов Types... Проверяется на этапе компиляции.
     *
     * @tparam Visitor Тип visitor'а (обычно core::overloaded<...>)
     * @param visitor Объект visitor
     * @return Результат, возвращённый visitor'ом
     */
    template <typename Visitor>
        requires detail::exhaustive_visitor<Visitor, std::variant<Types...>>
    decltype(auto) visit(Visitor&& visitor) {
        return std::visit(std::forward<Visitor>(visitor), data_);
    }

    /**
     * @brief Константная версия visit().
     */
    template <typename Visitor>
        requires detail::exhaustive_visitor<Visitor, const std::variant<Types...>>
    decltype(auto) visit(Visitor&& visitor) const {
        return std::visit(std::forward<Visitor>(visitor), data_);
    }

    std::size_t index() const noexcept { return data_.index(); }

    /// @}

private:
    std::variant<Types...> data_;
};

}  // namespace core
