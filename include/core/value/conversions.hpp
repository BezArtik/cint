// core/value/conversions.hpp

#include <charconv>
#include <concepts>
#include <optional>
#include <string_view>

namespace cint::core {

template <typename T>
concept numerical = std::integral<T> || std::floating_point<T>;

template <numerical T>
std::optional<T> to_numbers(std::string_view text) noexcept {
  auto num = T{};
  auto [ptr, ec] = std::from_chars(text.begin(), text.end(), num);
  if (ec != std::errc{} || ptr != text.end()) return std::nullopt;
  return num;
}

}  // namespace cint::core
