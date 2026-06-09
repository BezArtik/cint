// semantics/symbol_table.hpp


#pragma once
#include "core/token/token.hpp"
#include "core/token/token_types.hpp"
#include "core/utils/scoped_map.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>
#include <cstdint>


namespace semantics {

enum class symbol_kind : uint8_t {
	VARIABLE,
	FUNCTION
};

struct symbol_info {
	core::type type_{};
	symbol_kind kind_{};
};

class symbol_table {
public:

    void push();
    void pop() noexcept;
    void define(std::string_view name, core::type type);
    void define_function(std::string_view name, core::type func_type);
    std::optional<symbol_info> get(std::string_view name) const;
    bool contains_in_current_scope(std::string_view name) const;

private:
    core::scoped_map<symbol_info> scopes_;
};

} // namespace semantics