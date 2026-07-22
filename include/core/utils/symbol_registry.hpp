// core/utils/symbol_registry.hpp
#pragma once

#include "ast/statement.hpp"
#include "core/token/type.hpp"
#include "core/utils/builtins.hpp"

#include <span>
#include <string_view>
#include <variant>
#include <vector>

namespace core {

class symbol_registry {
public:
    using func_ptr = const ast::func_declaration*;
    using struct_ptr = const ast::struct_declaration*;

    using info_variant = std::variant<func_ptr, builtin_fn_ptr, struct_ptr>;

    struct entry {
        std::string_view name_;
        type type_;
        info_variant info_;
    };

    static symbol_registry build(std::span<const ast::stmt_ptr> ast, std::span<const builtin_def> builtins);

    const entry* find(std::string_view name) const noexcept;

    type resolve_type(const type& t) const;

private:
    symbol_registry() = default;

    void add_builtins(std::span<const builtin_def> builtins);
    void add_ast_entry(const ast::stmt_ptr& stmt);

    std::vector<entry> entries_;
};

}  // namespace core
