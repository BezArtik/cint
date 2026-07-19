// core/utils/function_registry.hpp
#pragma once

#include "ast/statement.hpp"
#include "core/utils/builtins.hpp"

#include <span>
#include <string_view>
#include <vector>

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

    static function_registry build(std::span<const ast::stmt_ptr> ast, std::span<const builtin_def> builtins);

    const entry* find(std::string_view name) const noexcept;

private:
    function_registry() = default;
    std::vector<entry> entries_;
};

}  // namespace core
