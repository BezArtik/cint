// parser/slot_allocator.cpp
#include "parser/slot_allocator.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string_view>

namespace cint {

void slot_allocator::push_scope() { scopes_.emplace_back(entries_.size(), counter_); }

void slot_allocator::pop_scope() {
  const auto b = scopes_.back();
  scopes_.pop_back();
  entries_.resize(b.entries_size_);
  counter_ = b.counter_;
}

void slot_allocator::begin_frame() {
  frames_.emplace_back(entries_.size(), counter_, max_slots_);
  counter_ = 0;
  max_slots_ = 0;
}

std::uint32_t slot_allocator::end_frame() {
  const auto frame_max = max_slots_;
  const auto f = frames_.back();
  frames_.pop_back();
  entries_.resize(f.entries_size_);
  counter_ = f.counter_;
  max_slots_ = f.max_slots_;
  return frame_max;
}

std::uint32_t slot_allocator::declare(std::string_view name) {
  const auto slot = counter_++;
  if (counter_ > max_slots_) max_slots_ = counter_;
  entries_.emplace_back(name, slot);
  return slot;
}

std::optional<std::uint32_t> slot_allocator::resolve(std::string_view name) const noexcept {
  const auto it = std::ranges::find(entries_.rbegin(), entries_.rend(), name, &entry::name_);
  if (it == entries_.rend()) return std::nullopt;
  return it->slot_;
}

}  // namespace cint
