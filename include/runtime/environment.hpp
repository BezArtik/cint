// runtime/environment.hpp


#pragma once
#include "runtime/value.hpp"
#include "core/utils/scoped_map.hpp"
#include "core/utils/builtins.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <optional>
#include <functional>


namespace runtime {

class environment {
public:

    environment() = default;

    void push_scope();
    void pop_scope();
    void define(const std::string& name, value val);
    bool assign(const std::string& name, value val);
    std::optional<value> get(const std::string& name) const;
	value* get_mut(const std::string& name);

    void define_builtin(const std::string& name, core::builtin_fn_ptr fn);
    std::optional<core::builtin_fn_ptr> get_builtin(const std::string& name) const;

private:

    core::scoped_map<value> values_;
    std::unordered_map<std::string, core::builtin_fn_ptr> builtins_;
};

} // namespace runtime