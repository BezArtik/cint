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

class symbol_table : public core::scoped_map<symbol_info> {
    using core::scoped_map<symbol_info>::define;
	using core::scoped_map<symbol_info>::find_scope;
public:

    void define(const std::string& name, core::type type);
    void define_function(const std::string& name, core::type func_type);
    bool mark_initialized(const std::string& name);
};

} // namespace semantics