/**
 * @file include/ast/variant_wrapper.hpp
 * @brief Обертка над std::variant для узлов AST, скрывающая работу с указателями.
 * @ingroup AST
 */

#pragma once

#include "core/memory/arena.hpp"
#include "core/utils/overloaded.hpp"

#include <concepts>
#include <functional>
#include <utility>
#include <variant>

namespace ast {

/**
 * @brief Концепт для проверки, входит ли тип в список.
 * @tparam T Проверяемый тип
 * @tparam Types Список допустимых типов
 */
template <typename T, typename... Types>
concept one_of = (std::same_as<T, Types> || ...);

/**
 * @brief Обертка над std::variant, автоматически разыменовывающая arena_ptr.
 * @ingroup AST
 *
 * Позволяет работать с узлами AST как со ссылками.
 *
 * @tparam Types Типы узлов, которые могут храниться в variant
 *
 * @invariant Всегда содержит валидный указатель на узел AST.
 */
template <typename... Types>
class variant_wrapper {
    /**
     * Тип, которым обернуты альтернативы Types.
     */
    template <typename T>
    using wrap = core::arena_ptr<T>;

public:
    static_assert(sizeof...(Types) > 0, "variant_wrapper must have at least one type");

    /// @name Конструкторы
    /// @{

    /**
     * @brief Конструирует обертку из wrap.
     * @tparam T Тип узла (должен быть одним из Types...)
     * @param ptr Указатель на узел в арене
     */
    template <typename T>
        requires one_of<T, Types...>
    variant_wrapper(wrap<T> ptr) : data_{std::move(ptr)} {}

    /// @}
    /// @name Проверка типа
    /// @{

    /**
     * @brief Проверяет, хранит ли обертка узел указанного типа.
     * @tparam T Проверяемый тип (должен быть одним из Types...)
     * @return true, если обертка хранит T
     */
    template <typename T>
        requires one_of<T, Types...>
    bool holds() const {
        return std::holds_alternative<wrap<T>>(data_);
    }

    /// @}

    /// @name Доступ к значению
    /// @{

    /**
     * @brief Возвращает ссылку на хранимый узел.
     * @tparam T Тип узла (должен быть одним из Types...)
     * @pre holds<T>() == true
     * @return Ссылка на узел
     * @throws std::bad_variant_access если тип не совпадает
     */
    template <typename T>
        requires one_of<T, Types...>
    T& get() {
        return *std::get<wrap<T>>(data_);
    }

    /**
     * @brief Константная версия get().
     */
    template <typename T>
        requires one_of<T, Types...>
    const T& get() const {
        return *std::get<wrap<T>>(data_);
    }

    /**
     * @brief Возвращает указатель на хранимый узел.
     * @tparam T Тип узла (должен быть одним из Types...)
     * @return Указатель на узел или nullptr, если тип не совпадает
     */
    template <typename T>
        requires one_of<T, Types...>
    T* get_if() {
        if (auto&& ptr = std::get_if<wrap<T>>(&data_)) return ptr->get();
        return nullptr;
    }

    /**
     * @brief Константная версия get_if().
     */
    template <typename T>
        requires one_of<T, Types...>
    const T* get_if() const {
        if (auto&& ptr = std::get_if<wrap<T>>(&data_)) return ptr->get();
        return nullptr;
    }

    /// @}

    /// @name Посещение
    /// @{

    /**
     * @brief Применяет visitor к хранимому узлу.
     *
     * Visitor должен иметь перегрузки operator() для всех типов Types...
     *
     * @tparam Visitor Тип visitor'а
     * @param visitor Объект visitor
     * @return Результат, возвращенный visitor'ом
     */
    template <typename Visitor>
    decltype(auto) visit(Visitor&& visitor) {
        return core::visit(
            [&](auto& ptr) -> decltype(auto) { return std::invoke(std::forward<Visitor>(visitor), *ptr); }, data_);
    }

    /**
     * @brief Константная версия visit().
     */
    template <typename Visitor>
    decltype(auto) visit(Visitor&& visitor) const {
        return core::visit(
            [&](const auto& ptr) -> decltype(auto) { return std::invoke(std::forward<Visitor>(visitor), *ptr); },
            data_);
    }

    /// @}
private:
    std::variant<wrap<Types>...> data_;
};

}  // namespace ast
