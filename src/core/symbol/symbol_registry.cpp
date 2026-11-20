// core/symbol/symbol_registry.cpp
#include "core/symbol/symbol_registry.hpp"

#include "ast/statement.hpp"
#include "core/error/error_codes.hpp"
#include "core/utils/overloaded.hpp"

#include <algorithm>

namespace cint::core {

symbol_registry::symbol_registry(std::span<const ast::statement> ast, diagnostic_sink& sink) {
    entries_.reserve(builtins.size() + ast.size());

    for (auto&& b : builtins) entries_.emplace_back(b.name_, &b);
    for (auto&& stmt : ast) add_ast_entry(stmt, sink);

    std::ranges::sort(entries_, {}, &entry::name_);
}
// clang-format off
void symbol_registry::add_ast_entry(const ast::statement& stmt, diagnostic_sink& sink) {
    std::visit(overloaded{
        [&](const core::arena_ptr<ast::func_declaration_stmt>& func) {
            auto name = func->name_.lexeme_;
            if (std::ranges::find(entries_, name, &entry::name_) != entries_.end()) {
                sink.report(func->loc_, error_code::redeclaration, name);
                return;
            }
            entries_.emplace_back(name, func.get());
        },
        [&](const core::arena_ptr<ast::struct_declaration_stmt>& strct) {
            auto name = strct->type_.name_;
            if (std::ranges::find(entries_, name, &entry::name_) != entries_.end()) {
                sink.report(strct->loc_, error_code::redeclaration, name);
                return;
            }
            entries_.emplace_back(name, strct.get());
        },
        [](const auto&) {}
    }, stmt);
}
// clang-format on

symbol_registry::const_iterator symbol_registry::find(std::string_view name) const noexcept {
    auto it = std::ranges::lower_bound(entries_, name, {}, &entry::name_);
    return (it != end() && it->name_ != name) ? end() : it;
}

}  // namespace cint::core
