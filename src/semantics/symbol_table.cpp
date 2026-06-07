// semantics/symbol_table.cpp


#include "semantics/symbol_table.hpp"
#include "core/token/token_types.hpp"

namespace semantics {

void symbol_table::push() { scopes_.push(); }
void symbol_table::pop() { scopes_.pop(); }

void symbol_table::define(const std::string& name, core::type type) {
    scopes_.define(name, { type, symbol_kind::VARIABLE });
}

void symbol_table::define_function(const std::string& name, core::type func_type) {
    scopes_.define(name, { func_type, symbol_kind::FUNCTION });
}

std::optional<symbol_info> symbol_table::get(const std::string& name) const {
    return scopes_.get(name);
}

bool symbol_table::contains_in_current_scope(const std::string& name) const {
    return scopes_.contains_in_current_scope(name);
}

} // namespace semantics