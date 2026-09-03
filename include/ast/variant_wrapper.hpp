/**
 * @file include/ast/variant_wrapper.hpp
 * @brief Обертка над std::variant для узлов AST, скрывающая работу с указателями.
 * @ingroup AST
 */

#pragma once

#include "core/memory/arena.hpp"
#include "core/utils/variant.hpp"

#include <functional>
#include <utility>

namespace ast {

/**
 * @brief Обертка над std::variant, автоматически разыменовывающая arena_ptr.
 * @ingroup AST
 *
 * Позволяет работать с узлами AST как со ссылками.
 * Делегирует хранение универсальному core::variant.
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
    bool holds() const noexcept {
        return data_.template holds<wrap<T>>();
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
    T& get() {
        return *data_.template get<wrap<T>>();
    }

    /**
     * @brief Константная версия get().
     */
    template <typename T>
    const T& get() const {
        return *data_.template get<wrap<T>>();
    }

    /**
     * @brief Возвращает указатель на хранимый узел.
     * @tparam T Тип узла (должен быть одним из Types...)
     * @return Указатель на узел или nullptr, если тип не совпадает
     */
    template <typename T>
    T* get_if() noexcept {
        if (auto&& ptr = data_.template get_if<wrap<T>>()) return ptr->get();
        return nullptr;
    }

    /**
     * @brief Константная версия get_if().
     */
    template <typename T>
    const T* get_if() const noexcept {
        if (auto&& ptr = data_.template get_if<wrap<T>>()) return ptr->get();
        return nullptr;
    }

    /// @}

    /// @name Посещение
    /// @{

    /**
     * @brief Применяет visitor к хранимому узлу.
     *
     * Visitor должен иметь перегрузки operator() для всех типов Types...
     * Автоматически разыменовывает arena_ptr перед вызовом visitor'а.
     *
     * @tparam Visitor Тип visitor'а
     * @param visitor Объект visitor
     * @return Результат, возвращённый visitor'ом
     */
    template <typename Visitor>
    decltype(auto) visit(Visitor&& visitor) {
        return data_.visit(
            [&](auto& ptr) -> decltype(auto) { return std::invoke(std::forward<Visitor>(visitor), *ptr); });
    }

    /**
     * @brief Константная версия visit().
     */
    template <typename Visitor>
    decltype(auto) visit(Visitor&& visitor) const {
        return data_.visit(
            [&](const auto& ptr) -> decltype(auto) { return std::invoke(std::forward<Visitor>(visitor), *ptr); });
    }

    /// @}
private:
    core::variant<wrap<Types>...> data_;
};

}  // namespace ast
