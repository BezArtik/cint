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
    using map_type = std::unordered_map<std::string_view, T, transparent_string_hash, transparent_string_equal>;
    using map_iterator = typename map_type::iterator;
    using const_map_iterator = typename map_type::const_iterator;

public:
    scoped_map() { push(); }

    void push() { scopes_.emplace_back(); }

    void pop() noexcept {
        assert(scopes_.size() > 1 && "Cannot pop global scope");
        scopes_.pop_back();
    }

    void define(std::string_view name, T value) { scopes_.back().bindings_[name] = std::move(value); }

    const T* get(std::string_view name) const noexcept {
        auto [scope, it] = find_in_scope(name);
        return scope ? &it->second : nullptr;
    }

    T* get(std::string_view name) noexcept {
        auto [scope, it] = find_in_scope(name);
        return scope ? &it->second : nullptr;
    }

    bool assign(std::string_view name, T value) {
        auto [scope, it] = find_in_scope(name);
        if (!scope) return false;
        it->second = std::move(value);
        return true;
    }

    bool contains_in_current_scope(std::string_view name) const noexcept {
        return scopes_.back().bindings_.contains(name);
    }

private:
    struct scope {
        map_type bindings_;
    };
    std::vector<scope> scopes_;

    template <typename ScopeType, typename MapIter>
    static std::pair<ScopeType*, MapIter> find_in_scope_impl(std::vector<scope>& scopes, std::string_view name) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->bindings_.find(name);
            if (found != it->bindings_.end()) return {&(*it), found};
        }
        return {nullptr, {}};
    }

    auto find_in_scope(std::string_view name) { return find_in_scope_impl<scope, map_iterator>(scopes_, name); }
    auto find_in_scope(std::string_view name) const {
        return find_in_scope_impl<const scope, const_map_iterator>(scopes_, name);
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
