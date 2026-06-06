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
	bool initialized_{};
};

class symbol_table {
public:

    void push();
    void pop();
    void define(const std::string& name, core::type type);
    void define_function(const std::string& name, core::type func_type);
    void mark_initialized(const std::string& name);
    std::optional<symbol_info> get(const std::string& name) const;
    bool contains_in_current_scope(const std::string& name) const;

private:
    core::scoped_map<symbol_info> scopes_;
};

} // namespace semantics