// core/symbol/symbol_registry.hpp
#pragma once
#include "ast/nodes.hpp"
#include "core/builtins/builtins.hpp"
#include "core/error/diagnostic.hpp"

#include <array>
#include <cstddef>
#include <memory_resource>
#include <span>
#include <string_view>
#include <vector>

namespace cint::core {

class symbol_registry {
  struct entry;
  using entries_t = std::pmr::vector<entry>;
  using const_iterator = typename entries_t::const_iterator;

 public:
  using func_ptr = ast::func_declaration_stmt*;
  using struct_ptr = ast::struct_declaration_stmt*;
  using builtin_func_ptr = const core::builtin_def*;

  symbol_registry() = default;
  symbol_registry(std::span<const ast::statement> ast, diagnostic_sink& sink);

  [[nodiscard]] auto begin() const noexcept { return entries_.begin(); }
  [[nodiscard]] auto end() const noexcept { return entries_.end(); }

  [[nodiscard]] const_iterator find(std::string_view name) const noexcept;

 private:
  struct entry {
    std::string_view name_;
    std::variant<func_ptr, builtin_func_ptr, struct_ptr> info_;
  };

  void add_ast_entry(const ast::statement& stmt, diagnostic_sink& sink);

  std::array<std::byte, 2048> buffer_;
  std::pmr::monotonic_buffer_resource mr_{buffer_.data(), buffer_.size()};
  entries_t entries_{&mr_};
};

}  // namespace cint::core
