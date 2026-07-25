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
struct variant_types_impl;

template <typename... Ts>
struct variant_types_impl<std::variant<Ts...>> {
    static consteval auto check() -> std::type_identity<std::variant<Ts...>> { return {}; }
};

}  // namespace detail

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
}(detail::variant_types_impl<std::remove_cvref_t<Variant>>::check());

/**
 * @brief std::visit с проверкой полноты обработки variant.
 * @ingroup CoreVisit
 *
 * Работает как std::visit, но требует, чтобы visitor обрабатывал
 * ВСЕ возможные типы variant. При неполном наборе обработчиков —
 * ошибка компиляции с понятным сообщением.
 *
 * Используется в:
 * - type_checker (проверка всех видов инструкций)
 * - interpreter (выполнение всех видов выражений)
 * - debug (вывод всех видов AST-узлов)
 *
 * @tparam Variant Тип variant'а (выводится автоматически)
 * @tparam Visitor Тип visitor'а (выводится автоматически)
 * @param vis Visitor с набором лямбд
 * @param var Variant для посещения
 * @return Результат, возвращённый visitor'ом.
 *
 */
template <typename Variant, typename Visitor>
    requires exhaustive_visitor<Visitor, Variant>
decltype(auto) visit(Visitor&& vis, Variant&& var) {
    return std::visit(std::forward<Visitor>(vis), std::forward<Variant>(var));
}

/**
 * @brief std::visit без проверки полноты (запрещён через = delete).
 * @ingroup CoreVisit
 *
 * При попытке использовать обычный std::visit (без exhaustive_visitor)
 * будет ошибка компиляции. Это заставляет всегда использовать
 * exhaustive-версию и предотвращает случайные пропуски типов.
 *
 * @note Удалённая перегрузка — жёсткое требование полноты для всего кода.
 */
template <typename Variant, typename Visitor>
    requires(!exhaustive_visitor<Visitor, Variant>)
decltype(auto) visit(Visitor&&, Variant&&) = delete;

}  // namespace core
