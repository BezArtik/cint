// core/utils/scoped_map.hpp


#pragma once
#include "core/utils/hash.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <optional>
#include <functional>
#include <stdexcept>
#include <algorithm>

namespace core {

template<typename T>
class scoped_map {
public:
    scoped_map() {
        scopes_.push_back(std::make_unique<scope>());
    }

    void push() {
        scopes_.push_back(std::make_unique<scope>());
    }

    void pop() noexcept {
        if (scopes_.size() > 1) scopes_.pop_back();
    }

    void define(std::string_view name, T value) {
        scopes_.back()->bindings_.emplace(name, std::move(value));
    }

    std::optional<T> get(std::string_view name) const {
        auto* scope = find_scope(name);
        if (!scope) return std::nullopt;
        auto it = scope->bindings_.find(name);
        if (it != scope->bindings_.end()) return it->second;
        return std::nullopt;
    }

    T* get_mut(std::string_view name) { 
        auto* scope = find_scope(name);
        if (!scope) return nullptr;
        auto it = scope->bindings_.find(name);
        return it != scope->bindings_.end() ? &it->second : nullptr;
    }

    bool contains_in_current_scope(std::string_view name) const noexcept {
        return scopes_.back()->bindings_.contains(name);
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

private:

    struct scope {
        std::unordered_map<std::string, T, core::string_hash, std::equal_to<>> bindings_;
    };
    std::vector<std::unique_ptr<scope>> scopes_;

    scope* find_scope(std::string_view name) {
        auto it = std::find_if(scopes_.rbegin(), scopes_.rend(),
            [&](const auto& s) { return s->bindings_.contains(name); });
        return it != scopes_.rend() ? it->get() : nullptr;
    }

    const scope* find_scope(std::string_view name) const {
        auto it = std::find_if(scopes_.rbegin(), scopes_.rend(),
            [&](const auto& s) { return s->bindings_.contains(name); });
        return it != scopes_.rend() ? it->get() : nullptr;
    }
};

}