// runtime/environment.cpp


#include "runtime/environment.hpp"
#include "core/utils/builtins.hpp"
#include <optional>
#include <stdexcept>

namespace runtime {

void environment::push_scope() {
    values_.push();
}

void environment::pop_scope() noexcept {
    values_.pop();
}

void environment::define(std::string_view name, core::value val) {
    values_.define(name, val);
}

bool environment::assign(std::string_view name, core::value val) {
    return values_.assign(name, val);
}

std::optional<core::value> environment::get(std::string_view name) const {
    return values_.get(name);
}

core::value* environment::get_mut(std::string_view name) {
	return values_.get_mut(name);
}

void environment::define_builtin(std::string_view name, core::builtin_fn_ptr fn) {
    builtins_.emplace(name, std::move(fn));
}

std::optional<core::builtin_fn_ptr> environment::get_builtin(std::string_view name) const {
    auto found = builtins_.find(name);
    if (found != builtins_.end()) return found->second;
    return std::nullopt;
}

bool environment::contains_in_current_scope(std::string_view name) const noexcept {
    return values_.contains_in_current_scope(name);
}

} // namespace runtime