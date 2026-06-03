// runtime/environment.cpp


#include "runtime/environment.hpp"
#include "core/utils/builtins.hpp"
#include <optional>
#include <stdexcept>

namespace runtime {

void environment::push_scope() {
    values_.push();
}

void environment::pop_scope() {
    values_.pop();
}

void environment::define(const std::string& name, value val) {
    values_.define(name, val);
}

bool environment::assign(const std::string& name, value val) {
    return values_.assign(name, val);
}

std::optional<value> environment::get(const std::string& name) const {
    return values_.get(name);
}

value* environment::get_mut(const std::string& name) {
	return values_.get_mut(name);
}

void environment::define_builtin(const std::string& name, core::builtin_fn_ptr fn) {
    builtins_[name] = std::move(fn);
}

std::optional<core::builtin_fn_ptr> environment::get_builtin(const std::string& name) const {
    auto found = builtins_.find(name);
    if (found != builtins_.end()) return found->second;
    return std::nullopt;
}

} // namespace runtime