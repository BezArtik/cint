// core/symbol/symbol_registry.cpp
#include "core/symbol/symbol_registry.hpp"

#include "ast/statement.hpp"
#include "core/builtins/builtins.hpp"
#include "core/utils/overloaded.hpp"

#include <algorithm>
#include <utility>
#include <vector>

namespace core {

symbol_registry symbol_registry::build(std::span<const ast::statement> ast) {
    symbol_registry registry;
    registry.entries_.reserve(core::builtins.size() + ast.size());

    for (auto&& b : core::builtins) registry.entries_.emplace_back(b.type_.function_name(), b.type_, b.impl_);

    for (auto&& stmt : ast) registry.add_ast_entry(stmt);

    std::ranges::sort(registry.entries_, {}, &entry::name_);

    return registry;
}
// clang-format off
void symbol_registry::add_ast_entry(const ast::statement& stmt) {
    stmt.visit(overloaded{
            [&](const ast::func_declaration_stmt& func) {
                auto&& name = func.type_.function_name();
                auto&& type = func.type_;

                auto&& it = std::ranges::find(entries_, name, &entry::name_);
                if (it != entries_.end()) {
                    if (std::holds_alternative<builtin_func_ptr>(it->info_)) {
                        it->type_ = std::move(type);
                        it->info_ = &func;
                    }
                } else {
                    entries_.emplace_back(name, std::move(type), &func);
                }
            },
            [&](const ast::struct_declaration_stmt& strct) {
                auto&& name = strct.type_.struct_name();
                auto&& it = std::ranges::find(entries_, name, &entry::name_);
                if (it == entries_.end()) entries_.emplace_back(name, strct.type_, &strct);
            },
            [](const auto&) {}
    });
}
// clang-format on

symbol_registry::const_iterator symbol_registry::find(std::string_view name) const noexcept {
    return std::ranges::lower_bound(entries_, name, {}, &entry::name_);
}

type symbol_registry::resolve_type(const type& t) const {
    if (t.is_struct()) {
        if (!t.struct_fields().empty()) {
            std::vector<type::field_t> resolved_fields;
            resolved_fields.reserve(t.struct_fields().size());

            for (auto&& [name, field_type] : t.struct_fields())
                resolved_fields.emplace_back(name, resolve_type(field_type));
            return type::struct_type(t.struct_name(), std::move(resolved_fields));
        }
        auto&& entry_it = find(t.struct_name());
        return (entry_it != end() && std::holds_alternative<struct_ptr>(entry_it->info_))
                   ? resolve_type(entry_it->type_)
                   : type::unknown_type();
    }
    if (t.is_array()) return type::array_type(resolve_type(t.element_type()), t.array_size());
    return t;
}

}  // namespace core
