// runtime/environment.hpp


#pragma once
#include "core/value/value.hpp"
#include "core/utils/scoped_map.hpp"
#include "core/utils/builtins.hpp"
#include "core/utils/hash.hpp"
#include <string_view>
#include <unordered_map>
#include <vector>
#include <memory>
#include <optional>

namespace runtime {

class environment {
public:

    environment() = default;

    void push_scope();
    void pop_scope() noexcept;
    void define(std::string_view name, core::value val);
    bool assign(std::string_view name, core::value val);
    std::optional<core::value> get(std::string_view name) const;
	core::value* get_mut(std::string_view name);
    void define_builtin(std::string_view name, core::builtin_fn_ptr fn);
    std::optional<core::builtin_fn_ptr> get_builtin(std::string_view name) const;
    bool contains_in_current_scope(std::string_view name) const noexcept;
    core::scoped_map<core::value>& scopes();
    const core::scoped_map<core::value>& scopes() const;

private:

    core::scoped_map<core::value> values_;
    std::unordered_map<std::string, core::builtin_fn_ptr, 
        core::string_hash, std::equal_to<>
    > builtins_;
};

} // namespace runtime