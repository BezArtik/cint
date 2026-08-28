// core/memory/arena.cpp

#include "core/memory/arena.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace core {

void* arena::allocate_raw(size_t size, size_t alignment) {
    auto&& ptr = align_ptr(current_, alignment);

    if (ptr >= end_ || size > static_cast<size_t>(end_ - ptr)) {
        if (size > BLOCK_SIZE) {
            auto&& block = std::make_unique_for_overwrite<std::byte[]>(size);
            auto&& block_ptr = block.get();
            ptr = align_ptr(block_ptr, alignment);
            current_ = block_ptr;
            end_ = block_ptr + size;
            blocks_.push_back(std::move(block));
        } else {
            allocate_block();
            ptr = align_ptr(current_, alignment);
        }
    }

    current_ = ptr + size;
    return ptr;
}

void arena::reset() noexcept {
    for (auto &&it = destructors_.rbegin(), endit = destructors_.rend(); it != endit; ++it) it->destroy_(it->obj_);

    destructors_.clear();

    blocks_.clear();
    current_ = nullptr;
    end_ = nullptr;
}

void arena::allocate_block() {
    auto&& block = std::make_unique_for_overwrite<std::byte[]>(BLOCK_SIZE);
    current_ = block.get();
    end_ = current_ + BLOCK_SIZE;
    blocks_.push_back(std::move(block));
}

std::byte* arena::align_ptr(std::byte* ptr, size_t alignment) noexcept {
    auto&& intptr = reinterpret_cast<uintptr_t>(ptr);
    auto&& aligned = (intptr + alignment - 1) & ~(alignment - 1);
    return reinterpret_cast<std::byte*>(aligned);
}

}  // namespace core
