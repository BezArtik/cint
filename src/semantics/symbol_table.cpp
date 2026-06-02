// semantics/symbol_table.cpp


#include "semantics/symbol_table.hpp"
#include "core/token/token_types.hpp"

namespace semantics {

void symbol_table::define(const std::string& name, core::type type) {
    symbol_info info{ std::move(type), symbol_kind::VARIABLE, false };
    scoped_map::define(name, std::move(info));
}

void symbol_table::define_function(const std::string& name, core::type func_type) {
    symbol_info info{ std::move(func_type), symbol_kind::FUNCTION, true };
    scoped_map::define(name, std::move(info));
}

bool symbol_table::mark_initialized(const std::string& name) {
	auto* scope = find_scope(name);
	if (!scope) { return false; }
	scope->bindings_[name].initialized_ = true;
	return true;
}

} // namespace semantics