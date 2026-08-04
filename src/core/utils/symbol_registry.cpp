// core/utils/symbol_registry.cpp
#include "core/utils/symbol_registry.hpp"

#include "ast/statement.hpp"
#include "core/utils/builtins.hpp"
#include "core/utils/overloaded.hpp"

#include <algorithm>
#include <utility>
#include <vector>

namespace core {

symbol_registry symbol_registry::build(std::span<const ast::node<ast::statement>> ast) {
    symbol_registry registry;
    registry.entries_.reserve(core::builtins.size() + ast.size());

    for (auto&& b : core::builtins) {
        auto&& type = type::function_type(b.return_type_, b.param_types_);
        registry.entries_.emplace_back(b.name_, std::move(type), b.impl_);
    }

    for (auto&& stmt : ast) registry.add_ast_entry(stmt);

    return registry;
}

// clang-format off
void symbol_registry::add_ast_entry(const ast::node<ast::statement>& stmt) {
    visit(overloaded{
            [this](const ast::func_declaration& func) {            
                std::vector<type> param_types;
                param_types.reserve(func.params_.size());
                std::ranges::transform(func.params_, std::back_inserter(param_types),
                        [](auto&& p) { return p.type_; });

                auto&& type = type::function_type(func.return_type_, std::move(param_types));

                auto&& it = std::ranges::find(entries_, func.name_.lexeme_, &entry::name_);
                if (it != entries_.end()) {
                    if (std::holds_alternative<builtin_fn_ptr>(it->info_)) {
                        it->type_ = std::move(type);
                        it->info_ = &func;
                    }
                } else {
                    entries_.emplace_back(func.name_.lexeme_, std::move(type), &func);
                }
            },
            [this](const ast::struct_declaration& strct) {
                auto&& name = strct.name_.lexeme_;
                auto&& it = std::ranges::find(entries_, name, &entry::name_);
                if (it == entries_.end()) entries_.emplace_back(name, strct.type_, &strct);
            },
            [](const auto&) {}},
    stmt->data_);
}
// clang-format on
const symbol_registry::entry* symbol_registry::find(std::string_view name) const noexcept {
    auto&& it = std::ranges::find(entries_, name, &entry::name_);
    return it != entries_.end() && it->name_ == name ? &*it : nullptr;
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
        auto&& entry = find(t.struct_name());
        return (entry && std::holds_alternative<struct_ptr>(entry->info_)) ? resolve_type(entry->type_)
                                                                           : type::unknown_type();
    }
    if (t.is_array()) return type::array_type(resolve_type(t.element_type()), t.array_size());
    return t;
}

}  // namespace core
