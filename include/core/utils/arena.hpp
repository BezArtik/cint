// core/utils/arena.hpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace core {

class arena {
    static constexpr size_t BLOCK_SIZE = 64 * 1024;

public:
    arena() = default;

    arena(const arena&) = delete;
    arena& operator=(const arena&) = delete;
    arena(arena&&) = delete;
    arena& operator=(arena&&) = delete;

    ~arena() { reset(); }

    template <typename T, typename... Args>
    T* allocate(Args&&... args) {
        auto* ptr = allocate_raw(sizeof(T), alignof(T));
        auto* obj = ::new (ptr) T(std::forward<Args>(args)...);

        if constexpr (!std::is_trivially_destructible_v<T>) destructors_.push_back([obj] { obj->~T(); });

        return obj;
    }

    void* allocate_raw(size_t size, size_t alignment) {
        auto* ptr = align_ptr(current_, alignment);
        auto remaining = static_cast<size_t>(end_ - ptr);

        if (size > remaining) {
            allocate_block();
            ptr = align_ptr(current_, alignment);
        }

        current_ = ptr + size;
        total_allocated_ += size;
        return ptr;
    }

    void reset() noexcept {
        for (auto it = destructors_.rbegin(); it != destructors_.rend(); ++it) (*it)();

        destructors_.clear();

        blocks_.clear();
        current_ = nullptr;
        end_ = nullptr;
        total_allocated_ = 0;
    }

private:
    void allocate_block() {
        auto block = std::make_unique_for_overwrite<char[]>(BLOCK_SIZE);
        current_ = block.get();
        end_ = current_ + BLOCK_SIZE;
        blocks_.push_back(std::move(block));
    }

    static char* align_ptr(char* ptr, size_t alignment) noexcept {
        auto intptr = reinterpret_cast<uintptr_t>(ptr);
        auto aligned = (intptr + alignment - 1) & ~(alignment - 1);
        return reinterpret_cast<char*>(aligned);
    }

    std::vector<std::unique_ptr<char[]>> blocks_;
    std::vector<std::function<void()>> destructors_;
    char* current_ = nullptr;
    char* end_ = nullptr;
    size_t total_allocated_ = 0;
};

class arena_memory_resource : public std::pmr::memory_resource {
public:
    arena_memory_resource(arena& arena) noexcept : arena_(arena) {}

private:
    void* do_allocate(size_t bytes, size_t alignment) override { return arena_.allocate_raw(bytes, alignment); }

    void do_deallocate(void*, size_t, size_t) noexcept override {}

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override { return this == &other; }

    arena& arena_;
};

template <typename T>
struct arena_deleter {
    void operator()(T*) const noexcept {}
};

template <typename T>
using arena_ptr = std::unique_ptr<T, arena_deleter<T>>;

}  // namespace core
