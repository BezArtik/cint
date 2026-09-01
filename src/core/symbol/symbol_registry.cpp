// core/symbol/symbol_registry.cpp
#include "core/symbol/symbol_registry.hpp"

#include "ast/statement.hpp"
#include "core/builtins/builtins.hpp"
#include "core/error/error_codes.hpp"
#include "core/error/error_report.hpp"
#include "core/utils/overloaded.hpp"

#include <algorithm>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace core {
// clang-format off
type symbol_registry::get_type(const_iterator it) const {
    return visit(overloaded{
            [](func_ptr f) { return f->type_; }, 
            [](builtin_func_ptr b) { return b->type_; },
            [](struct_ptr s) { return s->type_; }},
            it->info_);
}
// clang-format on
symbol_registry symbol_registry::build(std::span<const ast::statement> ast, error_reporter& reporter) {
    symbol_registry registry;
    registry.entries_.reserve(core::builtins.size() + ast.size());

    for (auto&& b : core::builtins) registry.entries_.emplace_back(b.type_.function_name(), &b);

    for (auto&& stmt : ast) registry.add_ast_entry(stmt, reporter);

    std::ranges::sort(registry.entries_, {}, &entry::name_);

    return registry;
}
// clang-format off
void symbol_registry::add_ast_entry(const ast::statement& stmt, error_reporter& reporter) {
    stmt.visit(overloaded{
            [&](const ast::func_declaration_stmt& func) {
                auto&& name = func.type_.function_name();
                auto&& it = std::ranges::find(entries_, name, &entry::name_);
                if (it != end()) {
                    reporter.error(func.loc_, error_code::redeclaration, name);
                    return;
                }
                entries_.emplace_back(name, &func);
            },
            [&](const ast::struct_declaration_stmt& strct) {
                auto&& name = strct.type_.struct_name();
                auto&& it = std::ranges::find(entries_, name, &entry::name_);
                if (it != end()) {
                    reporter.error(strct.loc_, error_code::redeclaration, name);
                    return;
                }
                entries_.emplace_back(name, &strct);
            },
            [](const auto&) {}
    });
}
// clang-format on

symbol_registry::const_iterator symbol_registry::find(std::string_view name) const noexcept {
    return std::ranges::lower_bound(entries_, name, {}, &entry::name_);
}

type symbol_registry::resolve_type(const type& t) const {
    std::unordered_set<std::string_view> resolving;
    return resolve_type_impl(t, resolving);
}

type symbol_registry::resolve_type_impl(const type& t, std::unordered_set<std::string_view>& resolving) const {
    if (t.is_struct()) {
        auto&& struct_name = t.struct_name();
        auto&& fields = t.struct_fields();

        if (!fields.empty()) {
            if (resolving.contains(struct_name)) return type::unknown_type();
            resolving.insert(struct_name);

            std::vector<type::field_t> resolved_fields;
            resolved_fields.reserve(fields.size());

            for (auto&& [name, field_type] : fields) {
                auto&& resolved_field = resolve_type_impl(field_type, resolving);
                if (resolved_field.is_unknown() && field_type.is_struct()) {
                    resolving.erase(struct_name);
                    return type::unknown_type();
                }
                resolved_fields.emplace_back(name, resolved_field);
            }

            resolving.erase(struct_name);
            return type::struct_type(struct_name, std::move(resolved_fields));
        }

        auto&& entry_it = find(struct_name);
        return entry_it != end() && std::holds_alternative<struct_ptr>(entry_it->info_)
                   ? resolve_type_impl(get_type(entry_it), resolving)
                   : type::unknown_type();
    }

    if (t.is_array()) {
        auto&& elem_type = resolve_type_impl(t.element_type(), resolving);
        if (elem_type.is_unknown() && t.element_type().is_struct()) return type::unknown_type();
        return type::array_type(elem_type, t.array_size());
    }

    return t;
}

}  // namespace core
