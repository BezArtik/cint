// core/memory/arena.hpp
#pragma once

#include <concepts>
#include <cstddef>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace cint::core {

class arena {
    static constexpr std::size_t BLOCK_SIZE = 16 * 1024;

public:
    arena() = default;

    arena(const arena&) = delete;
    arena& operator=(const arena&) = delete;
    arena(arena&&) = delete;
    arena& operator=(arena&&) = delete;
    ~arena() = default;

    [[nodiscard]] void* allocate(std::size_t size, std::size_t alignment);

private:
    std::vector<std::unique_ptr<std::byte[]>> blocks_;
    std::byte* current_{};
    std::byte* end_{};
};

template <typename T>
class arena_allocator {
public:
    using value_type = T;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;

    arena_allocator(arena& a) noexcept : arena_{&a} {}

    template <typename U>
    arena_allocator(const arena_allocator<U>& other) noexcept : arena_{other.arena_} {}

    [[nodiscard]] T* allocate(std::size_t n) {
        if (!arena_) throw std::bad_alloc{};
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T)) throw std::bad_array_new_length{};
        return static_cast<T*>(arena_->allocate(n * sizeof(T), alignof(T)));
    }

    void deallocate(T*, std::size_t) noexcept {}

    template <typename U>
    friend class arena_allocator;
    template <typename U>
    friend bool operator==(const arena_allocator<U>&, const arena_allocator<U>&) noexcept;

private:
    arena* arena_{};
};

template <typename T, typename U>
[[nodiscard]] bool operator==(const arena_allocator<T>& a, const arena_allocator<U>& b) noexcept {
    return a.arena_ == b.arena_;
}

template <typename T, typename U>
[[nodiscard]] bool operator!=(const arena_allocator<T>& a, const arena_allocator<U>& b) noexcept {
    return !(a == b);
}

template <typename T>
using arena_vector = std::vector<T, arena_allocator<T>>;

template <typename T>
struct arena_deleter {
    void operator()(T*) const noexcept {}
};

template <typename T>
using arena_ptr = std::unique_ptr<T, arena_deleter<T>>;

template <typename T, typename... Args>
    requires std::constructible_from<T, Args...>
[[nodiscard]] auto make_object(arena& arena, Args&&... args) {
    const auto ptr = arena.allocate(sizeof(T), alignof(T));
    const auto obj = std::construct_at<T>(static_cast<T*>(ptr), std::forward<Args>(args)...);
    return arena_ptr<T>{obj};
}

}  // namespace cint::core
