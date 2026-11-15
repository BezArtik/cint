// core/utils/variant.hpp
#pragma once

#include <concepts>
#include <type_traits>
#include <utility>
#include <variant>

namespace cint::core {

namespace detail {

template <typename T>
struct variant_types;

template <typename... Ts>
struct variant_types<std::variant<Ts...>> {
    static consteval std::type_identity<std::variant<Ts...>> check() { return {}; }
};

template <typename Visitor, typename Variant>
concept exhaustive_visitor = []<typename... Types>(std::type_identity<std::variant<Types...>>) {
    return (std::invocable<Visitor, Types> && ...);
}(variant_types<std::remove_cvref_t<Variant>>::check());

template <typename T, typename... Types>
concept one_of = (std::same_as<T, Types> || ...);

}  // namespace detail

template <typename... Types>
    requires(sizeof...(Types) > 0)
class variant {
public:
    variant() = default;

    template <typename T>
        requires detail::one_of<T, Types...>
    variant(T&& value) : data_{std::forward<T>(value)} {}

    template <typename T>
        requires detail::one_of<T, Types...>
    [[nodiscard]] auto holds() const noexcept {
        return std::holds_alternative<T>(data_);
    }

    template <typename T, typename Self>
        requires detail::one_of<T, Types...>
    [[nodiscard]] decltype(auto) get(this Self&& self) {
        return std::get<T>(std::forward<Self>(self).data_);
    }

    template <typename T, typename Self>
        requires detail::one_of<T, Types...>
    [[nodiscard]] decltype(auto) get_if(this Self&& self) noexcept {
        return std::get_if<T>(&std::forward<Self>(self).data_);
    }

    template <typename Visitor, typename Self>
        requires detail::exhaustive_visitor<Visitor, std::variant<Types...>>
    [[nodiscard]] decltype(auto) visit(this Self&& self, Visitor&& visitor) {
        return std::visit(std::forward<Visitor>(visitor), std::forward<Self>(self).data_);
    }

private:
    std::variant<Types...> data_;
};

}  // namespace cint::core
