// core/utils/arena.hpp
#pragma once

#include <cstddef>
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

    ~arena();

    template <typename T, typename... Args>
    T* allocate(Args&&... args) {
        auto* ptr = allocate_raw(sizeof(T), alignof(T));
        auto* obj = ::new (ptr) T(std::forward<Args>(args)...);

        if constexpr (!std::is_trivially_destructible_v<T>) destructors_.push_back([obj] { obj->~T(); });

        return obj;
    }

    void* allocate_raw(size_t size, size_t alignment);
    void reset() noexcept;

private:
    void allocate_block();
    static char* align_ptr(char* ptr, size_t alignment) noexcept;

    std::vector<std::unique_ptr<char[]>> blocks_;
    std::vector<std::function<void()>> destructors_;
    char* current_ = nullptr;
    char* end_ = nullptr;
    size_t total_allocated_ = 0;
};

class arena_memory_resource : public std::pmr::memory_resource {
public:
    arena_memory_resource(arena& arena) noexcept;

private:
    void* do_allocate(size_t bytes, size_t alignment) override;
    void do_deallocate(void*, size_t, size_t) noexcept override {}

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;

    arena& arena_;
};

template <typename T>
struct arena_deleter {
    void operator()(T*) const noexcept {}
};

template <typename T>
using arena_ptr = std::unique_ptr<T, arena_deleter<T>>;

}  // namespace core
