// core/utils/scoped_map.hpp

#pragma once

#include <cassert>
#include <cstddef>
#include <string_view>
#include <vector>

namespace core {

template <typename T>
class scoped_map {
    struct entry {
        std::string_view name_;
        T value_;
        size_t scope_depth_;
    };

public:
    scoped_map() { entries_.reserve(256); }

    void push() noexcept { ++current_depth_; }

    void pop() noexcept {
        assert(current_depth_ > 0 && "Cannot pop global scope");

        while (!entries_.empty() && entries_.back().scope_depth_ == current_depth_) { entries_.pop_back(); }
        --current_depth_;
    }

    void define(std::string_view name, T value) { entries_.push_back({name, std::move(value), current_depth_}); }

    T* get(std::string_view name) noexcept {
        for (auto it = entries_.rbegin(); it != entries_.rend(); ++it) {
            if (it->name_ == name) { return &it->value_; }
        }
        return nullptr;
    }

    const T* get(std::string_view name) const noexcept { return const_cast<scoped_map*>(this)->get(name); }

    bool assign(std::string_view name, T value) {
        auto* ptr = get(name);
        if (!ptr) return false;
        *ptr = std::move(value);
        return true;
    }

    bool contains_in_current_scope(std::string_view name) const noexcept {
        for (auto it = entries_.rbegin(); it != entries_.rend(); ++it) {
            if (it->scope_depth_ < current_depth_) break;
            if (it->name_ == name) return true;
        }
        return false;
    }

    size_t size() const noexcept { return entries_.size(); }
    size_t depth() const noexcept { return current_depth_; }

private:
    std::vector<entry> entries_;
    size_t current_depth_ = 0;
};

template <typename T>
class scope_guard {
public:
    scope_guard(scoped_map<T>& map) : map_(map) { map_.push(); }
    ~scope_guard() { map_.pop(); }
    scope_guard(const scope_guard&) = delete;
    scope_guard& operator=(const scope_guard&) = delete;

private:
    scoped_map<T>& map_;
};

}  // namespace core
