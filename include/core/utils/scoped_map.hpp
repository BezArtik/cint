// core/utils/scoped_map.hpp


#pragma once
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

    void pop() {
        if (scopes_.size() > 1) scopes_.pop_back();
    }

    void define(const std::string& name, T value) {
        scopes_.back()->bindings_[name] = std::move(value);
    }

    std::optional<T> get(const std::string& name) const {
        auto* scope = find_scope(name);
        if (scope) return scope->bindings_.at(name);
        return std::nullopt;
    }

    bool contains_in_current_scope(const std::string& name) const {
        return scopes_.back()->bindings_.contains(name);
    }

    bool assign(const std::string& name, T value) {
        auto* scope = find_scope(name);
        if (scope) {
            scope->bindings_[name] = std::move(value);
            return true;
        }
        return false;
    }

    void update_if_exists(const std::string& name, std::function<void(T&)> updater) {
        auto* scope = find_scope(name);
        if (scope) updater(scope->bindings_.at(name));
    }

private:

    struct scope {
        std::unordered_map<std::string, T> bindings_;
    };
    std::vector<std::unique_ptr<scope>> scopes_;

    scope* find_scope(const std::string& name) {
        auto it = std::find_if(scopes_.rbegin(), scopes_.rend(),
            [&](const auto& s) { return s->bindings_.contains(name); });
        return it != scopes_.rend() ? it->get() : nullptr;
    }

    const scope* find_scope(const std::string& name) const {
        auto it = std::find_if(scopes_.rbegin(), scopes_.rend(),
            [&](const auto& s) { return s->bindings_.contains(name); });
        return it != scopes_.rend() ? it->get() : nullptr;
    }

};

}