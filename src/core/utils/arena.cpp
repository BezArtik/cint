// core/utils/arena.cpp

#include "core/utils/arena.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace core {

arena::~arena() {
    reset();
}

void* arena::allocate_raw(size_t size, size_t alignment) {
    auto* ptr = align_ptr(current_, alignment);

    if (ptr >= end_ || size > static_cast<size_t>(end_ - ptr)) {
        if (size > BLOCK_SIZE) {
            auto block = std::make_unique_for_overwrite<char[]>(size);
            auto* block_ptr = block.get();
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
    for (auto it = destructors_.rbegin(); it != destructors_.rend(); ++it) (*it)();

    destructors_.clear();

    blocks_.clear();
    current_ = nullptr;
    end_ = nullptr;
}

void arena::allocate_block() {
    auto block = std::make_unique_for_overwrite<char[]>(BLOCK_SIZE);
    current_ = block.get();
    end_ = current_ + BLOCK_SIZE;
    blocks_.push_back(std::move(block));
}

char* arena::align_ptr(char* ptr, size_t alignment) noexcept {
    auto intptr = reinterpret_cast<uintptr_t>(ptr);
    auto aligned = (intptr + alignment - 1) & ~(alignment - 1);
    return reinterpret_cast<char*>(aligned);
}

arena_memory_resource::arena_memory_resource(arena& arena) noexcept : arena_(arena) {}

void* arena_memory_resource::do_allocate(size_t bytes, size_t alignment) {
    return arena_.allocate_raw(bytes, alignment);
}

bool arena_memory_resource::do_is_equal(const std::pmr::memory_resource& other) const noexcept {
    return this == &other;
}

}  // namespace core
