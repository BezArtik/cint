// core/utils/function_registry.hpp
#pragma once

#include "ast/statement.hpp"
#include "core/token/token_types.hpp"
#include "core/utils/builtins.hpp"

#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace ast {
struct func_declaration;
}  // namespace ast

namespace core {

class function_registry {
public:
    using func_ptr = const ast::func_declaration*;

    struct entry {
        std::string_view name_;
        core::type type_;
        func_ptr body_ = nullptr;
        builtin_fn_ptr builtin_ = nullptr;
    };

    static function_registry build(const std::vector<ast::stmt_ptr>& ast, std::span<const builtin_def> builtins);

    std::optional<entry> find(std::string_view name) const noexcept;

private:
    function_registry() = default;
    std::vector<entry> entries_;
};

}  // namespace core
