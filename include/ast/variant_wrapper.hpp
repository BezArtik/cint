/**
 * @file include/ast/variant_wrapper.hpp
 * @brief Обертка над std::variant для узлов AST, скрывающая работу с указателями.
 * @ingroup AST
 */

#pragma once

#include "core/utils/arena.hpp"
#include "core/utils/overloaded.hpp"

#include <variant>
#include <type_traits>

namespace ast {


 /**
 * @brief Вспомогательная структура для проверки, входит ли тип в список.
 * @tparam T Проверяемый тип
 * @tparam Types Список допустимых типов
 */
template <typename T, typename... Types>
struct is_one_of : std::disjunction<std::is_same<T, Types>...> {};

/**
 * @brief Вспомогательная переменная для удобства использования.
 */
template <typename T, typename... Types>
inline constexpr bool is_one_of_v = is_one_of<T, Types...>::value;   

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
public:
    static_assert(sizeof...(Types) > 0, "variant_wrapper must have at least one type");
    /// @name Конструкторы
    /// @{
    
    /**
     * @brief Конструирует обертку из core::arena_ptr.
     * @tparam T Тип узла (должен быть одним из Types...)
     * @param ptr Указатель на узел в арене
     */
    template <typename T>
    variant_wrapper(core::arena_ptr<T> ptr) : data_{std::move(ptr)} {
        static_assert(is_one_of_v<T, Types...>, 
                "T must be one of the variant_wrapper's types");
    }
    
    /// @}
    
    /// @name Проверка типа
    /// @{
    
    /**
     * @brief Проверяет, хранит ли обертка узел указанного типа.
     * @tparam T Проверяемый тип (должен быть одним из Types...)
     * @return true, если обертка хранит T
     */
    template <typename T>
    bool holds() const {
       static_assert(is_one_of_v<T, Types...>, 
               "T must be one of the variant_wrapper's types");
        return std::holds_alternative<core::arena_ptr<T>>(data_);
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
       static_assert(is_one_of_v<T, Types...>, 
               "T must be one of the variant_wrapper's types");
        return *std::get<core::arena_ptr<T>>(data_);
    }
    
    /**
     * @brief Константная версия get().
     */
    template <typename T>
    const T& get() const {
       static_assert(is_one_of_v<T, Types...>, 
               "T must be one of the variant_wrapper's types");
        return *std::get<core::arena_ptr<T>>(data_);
    }
    
    /**
     * @brief Возвращает указатель на хранимый узел.
     * @tparam T Тип узла (должен быть одним из Types...)
     * @return Указатель на узел или nullptr, если тип не совпадает
     */
    template <typename T>
    T* get_if() {
       static_assert(is_one_of_v<T, Types...>, 
               "T must be one of the variant_wrapper's types");
        if (auto&& ptr = std::get_if<core::arena_ptr<T>>(&data_)) return ptr->get();
        return nullptr;
    }
    
    /**
     * @brief Константная версия get_if().
     */
    template <typename T>
    const T* get_if() const {
       static_assert(is_one_of_v<T, Types...>, 
               "T must be one of the variant_wrapper's types");
        if (auto&& ptr = std::get_if<core::arena_ptr<T>>(&data_)) return ptr->get();
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
            [&](auto& ptr) -> decltype(auto) {
                return std::forward<Visitor>(visitor)(*ptr);
            },
            data_
        );
    }
    
    /**
     * @brief Константная версия visit().
     */
    template <typename Visitor>
    decltype(auto) visit(Visitor&& visitor) const {
        return core::visit(
            [&](const auto& ptr) -> decltype(auto) {
                return std::forward<Visitor>(visitor)(*ptr);
            },
            data_
        );
    }
    
    /// @}
private:
    std::variant<core::arena_ptr<Types>...> data_;
};

} // namespace ast
