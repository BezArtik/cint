// core/literal_pool.hpp
#pragma once
#include "core/value/value.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <vector>

namespace cint::core {

class literal_pool {
 public:
  std::uint32_t intern(value v) {
    values_.push_back(std::move(v));
    return static_cast<std::uint32_t>(values_.size() - 1);
  }

  [[nodiscard]] const value& get(std::uint32_t idx) const noexcept { return values_[idx]; }

 private:
  std::array<std::byte, 2048> buffer_{};
  std::pmr::monotonic_buffer_resource mr_{buffer_.data(), buffer_.size()};
  std::pmr::vector<value> values_{std::pmr::polymorphic_allocator{&mr_}};
};

}  // namespace cint::core
