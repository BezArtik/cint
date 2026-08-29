/**
 * @file include/core/utils/overloaded.hpp
 * @brief Утилиты для работы с std::variant и std::visit.
 * @ingroup CoreUtils
 */

#pragma once
#include <type_traits>
#include <variant>

namespace core {

/**
 * @brief Вспомогательный шаблон для создания visitor'а "на лету".
 * @ingroup CoreUtils
 *
 * Позволяет передать несколько лямбд в std::visit без явного
 * определения класса-visitor'а. Основан на множественном наследовании
 * от переданных типов лямбд и использовании их operator().
 *
 *
 * @tparam Ts Типы лямбд-обработчиков (по одному на каждый тип в variant)
 */
template <typename... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

/**
 * @defgroup CoreVisit Безопасный std::visit
 * @brief Обёртки над std::visit с проверкой полноты обработки variant.
 * @ingroup CoreUtils
 *
 * Предоставляет две версии visit:
 * - **exhaustive_visit**: требует обработчики для ВСЕХ типов variant (ошибка компиляции при неполноте)
 * - **non-exhaustive_visit**: разрешает частичную обработку (для отладки)
 */

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

}  // namespace detail

/**
 * @brief std::visit с проверкой полноты обработки variant.
 * @ingroup CoreVisit
 *
 * Работает как std::visit, но требует, чтобы visitor обрабатывал
 * ВСЕ возможные типы variant. При неполном наборе обработчиков —
 * ошибка компиляции.
 *
 * @tparam Variant Тип variant'а (выводится автоматически)
 * @tparam Visitor Тип visitor'а (выводится автоматически)
 * @param vis Visitor с набором лямбд
 * @param var Variant для посещения
 * @return Результат, возвращённый visitor'ом.
 *
 */
template <typename Variant, typename Visitor>
    requires detail::exhaustive_visitor<Visitor, Variant>
decltype(auto) visit(Visitor&& vis, Variant&& var) {
    return std::visit(std::forward<Visitor>(vis), std::forward<Variant>(var));
}

}  // namespace core
