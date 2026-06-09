// semantics/symbol_table.cpp


#include "semantics/symbol_table.hpp"
#include "core/token/token_types.hpp"

namespace semantics {

void symbol_table::push() { scopes_.push(); }
void symbol_table::pop() noexcept { scopes_.pop(); }

void symbol_table::define(std::string_view name, core::type type) {
    scopes_.define(name, { type, symbol_kind::VARIABLE });
}

void symbol_table::define_function(std::string_view name, core::type func_type) {
    scopes_.define(name, { func_type, symbol_kind::FUNCTION });
}

std::optional<symbol_info> symbol_table::get(std::string_view name) const {
    return scopes_.get(name);
}

bool symbol_table::contains_in_current_scope(std::string_view name) const {
    return scopes_.contains_in_current_scope(name);
}

} // namespace semantics