// core/utils/shared_data.hpp

#pragma once

#include <cstddef>
#include <utility>

namespace core {

template <typename T>
class shared_data {
    struct control_block {
        T data_;
        size_t ref_count_;

        template <typename... Args>
        control_block(Args&&... args) : data_(std::forward<Args>(args)...), ref_count_(1) {}
    };

public:
    shared_data() noexcept = default;

    template <typename... Args>
        requires std::constructible_from<T, Args...>
    shared_data(Args&&... args) : block_(new control_block(std::forward<Args>(args)...)) {}

    shared_data(const shared_data& other) noexcept : block_(other.block_) {
        if (block_) ++block_->ref_count_;
    }

    shared_data(shared_data&& other) noexcept : block_(std::exchange(other.block_, nullptr)) {}

    shared_data& operator=(const shared_data& other) noexcept {
        if (this == &other) return *this;
        release();
        block_ = other.block_;
        if (block_) ++block_->ref_count_;
        return *this;
    }

    shared_data& operator=(shared_data&& other) noexcept {
        if (this == &other) return *this;
        release();
        block_ = std::exchange(other.block_, nullptr);
        return *this;
    }

    ~shared_data() { release(); }

    const T& get() const noexcept { return block_->data_; }
    T& get() noexcept { return block_->data_; }

    const T* operator->() const noexcept { return &block_->data_; }
    T* operator->() noexcept { return &block_->data_; }

    const T& operator*() const noexcept { return block_->data_; }
    T& operator*() noexcept { return block_->data_; }

    bool is_unique() const noexcept { return block_ && block_->ref_count_ == 1; }
    bool is_shared() const noexcept { return block_ && block_->ref_count_ > 1; }
    bool empty() const noexcept { return block_ == nullptr; }
    size_t use_count() const noexcept { return block_ ? block_->ref_count_ : 0; }

    void detach() {
        if (is_shared()) {
            auto* old = block_;
            block_ = new control_block(old->data_);
            --old->ref_count_;
        }
    }

    shared_data clone() const {
        if (!block_) return shared_data{};
        return shared_data(block_->data_);
    }

    void swap(shared_data& other) noexcept { std::swap(block_, other.block_); }

private:
    void release() noexcept {
        if (block_ && --block_->ref_count_ == 0) {
            delete block_;
            block_ = nullptr;
        }
    }

    control_block* block_ = nullptr;
};

template <typename T>
void swap(shared_data<T>& a, shared_data<T>& b) noexcept {
    a.swap(b);
}

}  // namespace core
