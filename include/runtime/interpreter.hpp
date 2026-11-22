// runtime/interpreter.hpp
#pragma once
#include "ast/expression.hpp"
#include "ast/node.hpp"
#include "ast/statement.hpp"
#include "core/symbol/symbol_registry.hpp"
#include "core/token/token.hpp"
#include "core/utils/literal_pool.hpp"
#include "core/value/value.hpp"
#include "debug/debug_writer.hpp"

#include <limits>
#include <span>

namespace cint {

struct runtime_error : std::exception {
  core::error_code code_;
  core::location loc_;
  std::string message_;

  runtime_error(core::error_code c, core::location l, std::string m)
      : code_{c}, loc_{std::move(l)}, message_{std::move(m)} {}

  [[nodiscard]] const char* what() const noexcept override { return message_.c_str(); }
};

class interpreter {
 public:
  interpreter(std::span<const ast::statement> statements, const core::symbol_registry& registry,
              debug::debug_writer& writer, const core::literal_pool& literals);

 private:
  struct execution_result;

  execution_result execute(const ast::statement& stmt);
  execution_result execute(const ast::expression_stmt& stmt);
  execution_result execute(const ast::var_declaration_stmt& stmt);
  execution_result execute(const ast::func_declaration_stmt&);
  execution_result execute(const ast::struct_declaration_stmt&);
  execution_result execute(const ast::block_stmt& stmt);
  execution_result execute(const ast::while_stmt& stmt);
  execution_result execute(const ast::for_stmt& stmt);
  execution_result execute(const ast::if_stmt& stmt);
  execution_result execute(const ast::return_stmt& stmt);

  core::value evaluate(const ast::expression& expr);
  core::value evaluate(const ast::binary_expr& expr);
  core::value evaluate(const ast::assignment_expr& expr);
  core::value evaluate(const ast::unary_expr& expr);
  core::value evaluate(const ast::postfix_expr& expr);
  core::value evaluate(const ast::call_expr& expr);
  core::value evaluate(const ast::method_call_expr& expr);

  core::value& evaluate_lvalue(const ast::expression& expr, core::location loc, bool for_write);

  bool is_lvalue(const ast::expression& e);

  core::value call(core::symbol_registry::builtin_func_ptr ptr, std::span<const core::value> args);
  core::value call(core::symbol_registry::func_ptr ptr, std::span<const core::value> args);
  core::value call(core::symbol_registry::struct_ptr ptr, std::span<const core::value> args);

  const core::symbol_registry& registry_;
  debug::debug_writer& writer_;
  const core::literal_pool& literals_;
  const bool exec_enabled_debug_{};
  const bool ret_enabled_debug_{};
  std::vector<core::value> slots_;
  std::size_t frame_base_{};
  std::size_t this_idx_{MAX_THIS_IDX};
  std::uint32_t recursion_depth_{};

  static constexpr std::uint32_t MAX_RECURSION_DEPTH{250};
  static constexpr std::size_t MAX_THIS_IDX{std::numeric_limits<std::size_t>::max()};
};

}  // namespace cint
