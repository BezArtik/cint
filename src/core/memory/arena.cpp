// core/memory/arena.cpp

#include "core/memory/arena.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace cint::core {

namespace {

std::byte* align_ptr(std::byte* ptr, std::size_t alignment) noexcept {
    auto intptr = reinterpret_cast<std::uintptr_t>(ptr);
    auto aligned = (intptr + alignment - 1) & ~(alignment - 1);
    return reinterpret_cast<std::byte*>(aligned);
}

}  // namespace

void* arena::allocate_raw(std::size_t size, std::size_t alignment) {
    if (size == 0) size = 1;
    if (alignment == 0) alignment = 1;

    if (size + alignment - 1 > BLOCK_SIZE) {
        auto block = std::make_unique_for_overwrite<std::byte[]>(size + alignment - 1);
        auto block_ptr = block.get();
        auto ptr = align_ptr(block_ptr, alignment);
        blocks_.push_back(std::move(block));
        return ptr;
    }

    auto ptr = align_ptr(current_, alignment);
    if (ptr >= end_ || size > static_cast<std::size_t>(end_ - ptr)) {
        auto block = std::make_unique_for_overwrite<std::byte[]>(BLOCK_SIZE);
        current_ = block.get();
        end_ = current_ + BLOCK_SIZE;
        blocks_.push_back(std::move(block));
        ptr = align_ptr(current_, alignment);
    }

    current_ = ptr + size;
    return ptr;
}

void arena::reset() noexcept {
    for (auto it = destructors_.rbegin(), endit = destructors_.rend(); it != endit; ++it) it->destroy_(it->obj_);
    destructors_.clear();

    blocks_.clear();
    current_ = nullptr;
    end_ = nullptr;
}

}  // namespace cint::core
