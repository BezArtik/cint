// core/utils/small_vector.hpp

#pragma once

#include <cstddef>
#include <vector>

namespace core {

template <typename T, size_t N>
class small_vector {
    template <typename U>
    struct stack_allocator {
        using value_type = U;

        stack_allocator(std::byte* buffer, bool* used) noexcept : buffer_(buffer), used_(used) {}

        template <typename V>
        stack_allocator(const stack_allocator<V>& other) noexcept : buffer_(other.buffer_), used_(other.used_) {}

        template <typename V>
        struct rebind {
            using other = stack_allocator<V>;
        };

        U* allocate(size_t n) {
            if constexpr (std::is_same_v<U, T>) {
                if (!*used_ && n <= N) {
                    *used_ = true;
                    return reinterpret_cast<U*>(buffer_);
                }
            }
            return static_cast<U*>(::operator new(n * sizeof(U)));
        }

        void deallocate(U* p, size_t) noexcept {
            if constexpr (std::is_same_v<U, T>) {
                if (p == reinterpret_cast<U*>(buffer_)) {
                    *used_ = false;
                    return;
                }
            }
            ::operator delete(p);
        }

        template <typename V>
        bool operator==(const stack_allocator<V>& other) const noexcept {
            return buffer_ == other.buffer_;
        }

        template <typename V>
        bool operator!=(const stack_allocator<V>& other) const noexcept {
            return !(*this == other);
        }

        std::byte* buffer_;
        bool* used_;
    };

public:
    using container_type = std::vector<T, stack_allocator<T>>;
    using value_type = typename container_type::value_type;
    using pointer = typename container_type::pointer;
    using const_pointer = typename container_type::const_pointer;
    using reference = typename container_type::reference;
    using const_reference = typename container_type::const_reference;
    using iterator = typename container_type::iterator;
    using const_iterator = typename container_type::const_iterator;
    using reverse_iterator = typename container_type::reverse_iterator;
    using const_reverse_iterator = typename container_type::const_reverse_iterator;
    using size_type = typename container_type::size_type;

    small_vector() : vec_(stack_allocator<value_type>(buffer_, &used_)) { vec_.reserve(N); }

    void push_back(const_reference value) { vec_.push_back(value); }
    void push_back(value_type&& value) { vec_.push_back(std::move(value)); }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        vec_.emplace_back(std::forward<Args>(args)...);
    }

    void reserve(size_type n) { vec_.reserve(n); }

    reference operator[](size_type i) noexcept { return vec_[i]; }
    const_reference operator[](size_type i) const noexcept { return vec_[i]; }

    reference back() noexcept { return vec_.back(); }
    const_reference back() const noexcept { return vec_.back(); }

    pointer data() noexcept { return vec_.data(); }
    const_pointer data() const noexcept { return vec_.data(); }

    size_type size() const noexcept { return vec_.size(); }
    size_type capacity() const noexcept { return vec_.capacity(); }
    bool empty() const noexcept { return vec_.empty(); }

    void pop_back() noexcept { vec_.pop_back(); }
    void clear() noexcept { vec_.clear(); }

    iterator begin() noexcept { return vec_.begin(); }
    iterator end() noexcept { return vec_.end(); }
    const_iterator begin() const noexcept { return vec_.begin(); }
    const_iterator end() const noexcept { return vec_.end(); }
    reverse_iterator rbegin() noexcept { return vec_.rbegin(); }
    reverse_iterator rend() noexcept { return vec_.rend(); }
    const_reverse_iterator rbegin() const noexcept { return vec_.rbegin(); }
    const_reverse_iterator rend() const noexcept { return vec_.rend(); }

private:
    alignas(value_type) std::byte buffer_[N * sizeof(value_type)];
    bool used_ = false;
    container_type vec_;
};

}  // namespace core
