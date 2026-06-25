// core/utils/scoped_map.hpp

#pragma once
#include "core/utils/hash.hpp"

#include <algorithm>
#include <cassert>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace core {

template <typename T>
class scoped_map {
public:
    scoped_map() { push(); }

    void push() { scopes_.emplace_back(); }

    void pop() noexcept {
        assert(scopes_.size() > 1 && "Cannot pop global scope");
        scopes_.pop_back();
    }

    void define(std::string_view name, T value) { scopes_.back().bindings_[name] = std::move(value); }

    const T* get(std::string_view name) const noexcept {
        const auto* scope = find_scope(name);
        if (!scope) return nullptr;
        auto it = scope->bindings_.find(name);
        return it != scope->bindings_.end() ? &it->second : nullptr;
    }

    T* get(std::string_view name) noexcept {
        auto* scope = find_scope(name);
        if (!scope) return nullptr;
        auto it = scope->bindings_.find(name);
        return it != scope->bindings_.end() ? &it->second : nullptr;
    }

    bool assign(std::string_view name, T value) {
        auto* scope = find_scope(name);
        if (!scope) return false;
        auto it = scope->bindings_.find(name);
        if (it != scope->bindings_.end()) {
            it->second = std::move(value);
            return true;
        }
        return false;
    }

    bool contains_in_current_scope(std::string_view name) const noexcept {
        return scopes_.back().bindings_.contains(name);
    }

private:
    struct scope {
        std::unordered_map<std::string_view, T, transparent_string_hash, transparent_string_equal> bindings_;
    };
    std::vector<scope> scopes_;

    scope* find_scope(std::string_view name) {
        auto it =
            std::find_if(scopes_.rbegin(), scopes_.rend(), [&](const auto& s) { return s.bindings_.contains(name); });
        return it != scopes_.rend() ? &(*it) : nullptr;
    }

    const scope* find_scope(std::string_view name) const {
        auto it =
            std::find_if(scopes_.rbegin(), scopes_.rend(), [&](const auto& s) { return s.bindings_.contains(name); });
        return it != scopes_.rend() ? &(*it) : nullptr;
    }
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
