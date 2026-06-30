// core/utils/overloaded.hpp

#pragma once
#include <type_traits>
#include <variant>

namespace core {

template <typename... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

template <typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

namespace detail {
template <typename T>
struct variant_types_impl;
template <typename... Ts>
struct variant_types_impl<std::variant<Ts...>> {
    static consteval auto check() -> std::type_identity<std::variant<Ts...>> { return {}; }
};
}  // namespace detail

template <typename Visitor, typename Variant>
concept exhaustive_visitor = []<typename... Types>(std::type_identity<std::variant<Types...>>) {
    return (std::invocable<Visitor, Types> && ...);
}(detail::variant_types_impl<std::remove_cvref_t<Variant>>::check());

template <typename Variant, typename Visitor>
    requires exhaustive_visitor<Visitor, Variant>
decltype(auto) visit(Visitor&& vis, Variant&& var) {
    return std::visit(std::forward<Visitor>(vis), std::forward<Variant>(var));
}

template <typename Variant, typename Visitor>
    requires(!exhaustive_visitor<Visitor, Variant>)
decltype(auto) visit(Visitor&&, Variant&&) = delete;

}  // namespace core
