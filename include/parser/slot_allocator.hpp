// parser/slot_allocator.hpp
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <optional>
#include <string_view>

namespace cint {

class slot_allocator {
 public:
  slot_allocator() = default;

  void push_scope();
  void pop_scope();
  void begin_frame();
  [[nodiscard]] std::uint32_t end_frame();
  [[nodiscard]] std::uint32_t declare(std::string_view name);
  [[nodiscard]] std::optional<std::uint32_t> resolve(std::string_view name) const noexcept;

 private:
  struct entry {
    std::string_view name_;
    std::uint32_t slot_;
  };
  struct scope_boundary {
    std::size_t entries_size_;
    std::uint32_t counter_;
  };
  struct frame_boundary {
    std::size_t entries_size_;
    std::uint32_t counter_;
    std::uint32_t max_slots_;
  };

  std::array<std::byte, 2048> buffer_{};
  std::pmr::monotonic_buffer_resource mr_{buffer_.data(), buffer_.size()};

  std::pmr::vector<entry> entries_{&mr_};
  std::pmr::vector<scope_boundary> scopes_{&mr_};
  std::pmr::vector<frame_boundary> frames_{&mr_};
  std::uint32_t counter_{};
  std::uint32_t max_slots_{};
};

}  // namespace cint
