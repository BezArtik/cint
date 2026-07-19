// core/function_registry.cpp
#include "core/utils/function_registry.hpp"

#include "ast/statement.hpp"

#include <algorithm>

namespace core {

function_registry function_registry::build(std::span<const ast::stmt_ptr> ast, std::span<const builtin_def> builtins) {
    function_registry registry;
    registry.entries_.reserve(builtins.size());

    for (const auto& b : builtins) {
        auto type = core::type::function_type(b.return_type_, b.param_types_);
        registry.entries_.emplace_back(b.name_, std::move(type), nullptr, b.impl_);
    }

    for (const auto& stmt : ast) {
        if (auto* func = std::get_if<ast::func_declaration>(&stmt->data_)) {
            std::vector<core::type> param_types;
            param_types.reserve(func->params_.size());
            std::ranges::transform(func->params_, std::back_inserter(param_types),
                                   [](const auto& p) { return p.type_; });

            auto type = core::type::function_type(func->return_type_, std::move(param_types));

            auto it = std::ranges::find(registry.entries_, func->name_.lexeme_, &entry::name_);
            if (it != registry.entries_.end()) {
                it->type_ = std::move(type);
                it->body_ = func;
                it->builtin_ = nullptr;
            } else {
                registry.entries_.emplace_back(func->name_.lexeme_, std::move(type), func, nullptr);
            }
        }
    }

    std::ranges::sort(registry.entries_, {}, &entry::name_);
    return registry;
}

const function_registry::entry* function_registry::find(std::string_view name) const noexcept {
    auto it = std::ranges::lower_bound(entries_, name, {}, &entry::name_);
    return it != entries_.end() && it->name_ == name ? &*it : nullptr;
}

}  // namespace core
