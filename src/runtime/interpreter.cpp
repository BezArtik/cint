// runtime/interpreter.cpp

#include "runtime/interpreter.hpp"

#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/builtins/builtins.hpp"
#include "core/error/diagnostic.hpp"
#include "core/error/error_codes.hpp"
#include "core/symbol/symbol_registry.hpp"
#include "core/token/token_types.hpp"
#include "core/utils/literal_pool.hpp"
#include "core/utils/overloaded.hpp"
#include "core/value/value.hpp"
#include "debug/debug_writer.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <memory_resource>
#include <ranges>
#include <utility>
#include <vector>

namespace cint {

namespace {

template <typename... Args>
[[noreturn]] void raise_error(core::location loc, core::error_code code, Args&&... args) {
  throw runtime_error{code, loc, core::format_message(code, std::forward<Args>(args)...)};
}

}  // namespace

using tt = core::token_type;
using err = core::error_code;

struct interpreter::execution_result {
  enum class kind : std::uint8_t { normal, return_ };

  kind kind_{kind::normal};
  core::value value_;

  static execution_result normal() { return {kind::normal, {}}; }
  static execution_result return_(core::value v) { return {kind::return_, std::move(v)}; }

  bool is_return() const noexcept { return kind_ == kind::return_; }
};

interpreter::interpreter(std::span<const ast::statement> statements, const core::symbol_registry& registry,
                         debug::debug_writer& writer, const core::literal_pool& literals)
    : registry_{registry},
      writer_{writer},
      literals_{literals},
      exec_enabled_debug_{writer.enabled(debug::trace_level::execution)},
      ret_enabled_debug_{writer.enabled(debug::trace_level::returns)} {
  for (auto&& stmt : statements) {
    if (!std::holds_alternative<core::arena_ptr<ast::func_declaration_stmt>>(stmt)) execute(stmt);
  }
}

interpreter::execution_result interpreter::execute(const ast::statement& stmt) {
  if (exec_enabled_debug_) [[unlikely]]
    writer_.print(stmt);
  return std::visit([&](const auto& s) { return execute(*s); }, stmt);
}

interpreter::execution_result interpreter::execute(const ast::expression_stmt& stmt) {
  std::ignore = evaluate(stmt.expr_);
  return execution_result::normal();
}

interpreter::execution_result interpreter::execute(const ast::var_declaration_stmt& stmt) {
  const auto idx = frame_base_ + stmt.slot_;
  if (idx >= slots_.size()) slots_.resize(idx + 1);
  slots_[idx] = evaluate(stmt.initializer_);
  return execution_result::normal();
}

interpreter::execution_result interpreter::execute(const ast::func_declaration_stmt&) {
  return execution_result::normal();
}

interpreter::execution_result interpreter::execute(const ast::struct_declaration_stmt&) {
  return execution_result::normal();
}

interpreter::execution_result interpreter::execute(const ast::block_stmt& stmt) {
  for (auto&& s : stmt.statements_) {
    auto&& res = execute(s);
    if (res.is_return()) return res;
  }
  return execution_result::normal();
}

interpreter::execution_result interpreter::execute(const ast::while_stmt& stmt) {
  while (true) {
    auto&& cond = evaluate(stmt.condition_);
    if (!cond.to_bool()) break;

    auto&& res = execute(stmt.block_);
    if (res.is_return()) return res;
  }
  return execution_result::normal();
}

interpreter::execution_result interpreter::execute(const ast::for_stmt& stmt) {
  if (stmt.initializer_) execute(*stmt.initializer_);
  while (true) {
    if (stmt.condition_) {
      auto&& cond = evaluate(*stmt.condition_);
      if (!cond.to_bool()) break;
    }
    auto&& result = execute(stmt.block_);
    if (result.is_return()) return result;
    if (stmt.increment_) evaluate(*stmt.increment_);
  }
  return execution_result::normal();
}

interpreter::execution_result interpreter::execute(const ast::if_stmt& stmt) {
  auto&& cond = evaluate(stmt.condition_);
  if (cond.to_bool()) {
    return execute(stmt.then_block_);
  } else if (stmt.else_block_) {
    return execute(*stmt.else_block_);
  }
  return execution_result::normal();
}

interpreter::execution_result interpreter::execute(const ast::return_stmt& stmt) {
  auto ret_val = core::value{};
  if (stmt.value_) ret_val = evaluate(*stmt.value_);
  return execution_result::return_(std::move(ret_val));
}

bool interpreter::is_lvalue(const ast::expression& e) {
  return std::visit(
      core::overloaded{[](const core::arena_ptr<ast::variable_expr>&) { return true; },
                       [&](const core::arena_ptr<ast::member_access_expr>& m) { return is_lvalue(m->object_); },
                       [](const auto&) { return false; }},
      e);
}

core::value& interpreter::evaluate_lvalue(const ast::expression& expr, core::location loc, bool for_write) {
  return std::visit(core::overloaded{[&](const core::arena_ptr<ast::variable_expr>& e) -> core::value& {
                                       if (e->name_.lexeme_ == "this") {
                                         if (this_idx_ == MAX_THIS_IDX) [[unlikely]]
                                           raise_error(e->name_.loc_, err::this_outside_method);
                                         return slots_[this_idx_];
                                       }
                                       return slots_[frame_base_ + e->slot_];
                                     },
                                     [&](const core::arena_ptr<ast::member_access_expr>& e) -> core::value& {
                                       auto& obj = evaluate_lvalue(e->object_, e->member_.loc_, for_write);
                                       if (!obj.is_struct()) [[unlikely]]
                                         raise_error(e->member_.loc_, err::not_a_struct);
                                       if (for_write) obj.make_unique_struct();
                                       auto&& st = obj.to_struct();
                                       auto&& names = st.node_->field_names_;
                                       const auto it = std::ranges::find(names, e->member_.lexeme_);
                                       if (it == names.end()) [[unlikely]]
                                         raise_error(e->member_.loc_, err::no_such_field, st.node_->name_,
                                                     e->member_.lexeme_);
                                       return st.fields_[std::distance(names.begin(), it)];
                                     },
                                     [&](const auto&) -> core::value& { raise_error(loc, err::not_an_lvalue); }},
                    expr);
}

core::value interpreter::evaluate(const ast::expression& expr) {
  return std::visit(
      core::overloaded{
          [&](const core::arena_ptr<ast::literal_expr>& e) { return literals_.get(e->pool_idx_); },
          [&](const core::arena_ptr<ast::variable_expr>& e) { return evaluate_lvalue(expr, e->name_.loc_, false); },
          [&](const core::arena_ptr<ast::member_access_expr>& e) {
            if (is_lvalue(e->object_)) return evaluate_lvalue(expr, e->member_.loc_, false);
            auto&& obj = evaluate(e->object_);
            if (!obj.is_struct()) [[unlikely]]
              raise_error(e->member_.loc_, err::not_a_struct);
            auto&& st = obj.to_struct();
            auto&& names = st.node_->field_names_;
            const auto it = std::ranges::find(names, e->member_.lexeme_);
            if (it == names.end()) [[unlikely]]
              raise_error(e->member_.loc_, err::no_such_field, st.node_->name_, e->member_.lexeme_);
            return st.fields_[std::distance(names.begin(), it)];
          },
          [&](const core::arena_ptr<ast::binary_expr>& e) { return evaluate(*e); },
          [&](const core::arena_ptr<ast::assignment_expr>& e) { return evaluate(*e); },
          [&](const core::arena_ptr<ast::unary_expr>& e) { return evaluate(*e); },
          [&](const core::arena_ptr<ast::postfix_expr>& e) { return evaluate(*e); },
          [&](const core::arena_ptr<ast::call_expr>& e) { return evaluate(*e); },
          [&](const core::arena_ptr<ast::method_call_expr>& e) { return evaluate(*e); },
      },
      expr);
}

core::value interpreter::evaluate(const ast::binary_expr& expr) {
  const auto [type, op, loc] = expr.op_;
  try {
    auto&& left = evaluate(expr.left_);

    if (type == tt::LOGICAL_AND) {
      if (!left.to_bool()) return false;
      return evaluate(expr.right_).to_bool();
    }
    if (type == tt::LOGICAL_OR) {
      if (left.to_bool()) return true;
      return evaluate(expr.right_).to_bool();
    }

    auto&& right = evaluate(expr.right_);
    switch (type) {
      case tt::PLUS:
        return left + right;
      case tt::MINUS:
        return left - right;
      case tt::STAR:
        return left * right;
      case tt::SLASH:
        return left / right;
      case tt::PERCENT:
        return left % right;
      case tt::EQUAL_EQUAL:
        return left == right;
      case tt::BANG_EQUAL:
        return left != right;
      case tt::LESS:
        return left < right;
      case tt::LESS_EQUAL:
        return left <= right;
      case tt::GREATER:
        return left > right;
      case tt::GREATER_EQUAL:
        return left >= right;
      case tt::BIT_AND:
        return left & right;
      case tt::BIT_OR:
        return left | right;
      case tt::XOR:
        return left ^ right;
      case tt::SHL:
        return left << right;
      case tt::SHR:
        return left >> right;
      default:
        raise_error(loc, err::unsupported_binary_operator);
    }
  } catch (const core::value_error& e) {
    raise_error(loc, e.code_, op);
  }
}

core::value interpreter::evaluate(const ast::assignment_expr& expr) {
  const auto [type, op, loc] = expr.op_;
  auto&& right = evaluate(expr.value_);
  auto&& target = evaluate_lvalue(expr.target_, loc, true);

  try {
    switch (type) {
      case tt::EQUAL:
        target = std::move(right);
        break;
      case tt::PLUS_EQUAL:
        target = target + right;
        break;
      case tt::MINUS_EQUAL:
        target = target - right;
        break;
      case tt::STAR_EQUAL:
        target = target * right;
        break;
      case tt::SLASH_EQUAL:
        target = target / right;
        break;
      case tt::PERCENT_EQUAL:
        target = target % right;
        break;
      case tt::BIT_AND_EQUAL:
        target = target & right;
        break;
      case tt::BIT_OR_EQUAL:
        target = target | right;
        break;
      case tt::XOR_EQUAL:
        target = target ^ right;
        break;
      case tt::SHL_EQUAL:
        target = target << right;
        break;
      case tt::SHR_EQUAL:
        target = target >> right;
        break;
      default:
        break;
    }
  } catch (const core::value_error& e) {
    raise_error(loc, e.code_, op);
  }
  return target;
}

core::value interpreter::evaluate(const ast::unary_expr& expr) {
  const auto [type, op, loc] = expr.op_;
  try {
    switch (type) {
      case tt::INCREMENT: {
        auto&& t = evaluate_lvalue(expr.operand_, loc, true);
        t.is_int() ? t = t + core::value::int_t{1} : t = t + core::value::double_t{1};
        return t;
      }
      case tt::DECREMENT: {
        auto&& t = evaluate_lvalue(expr.operand_, loc, true);
        t.is_int() ? t = t - core::value::int_t{1} : t = t - core::value::double_t{1};
        return t;
      }
      default:
        break;
    }

    auto&& operand = evaluate(expr.operand_);
    switch (type) {
      case tt::MINUS:
        return -operand;
      case tt::BANG:
        return !operand;
      case tt::BIT_NOT:
        return ~operand;
      default:
        raise_error(loc, err::unsupported_unary_operator);
    }
  } catch (const core::value_error& e) {
    raise_error(loc, e.code_, op);
  }
}

core::value interpreter::evaluate(const ast::postfix_expr& expr) {
  const auto [type, op, loc] = expr.op_;
  try {
    switch (type) {
      case tt::INCREMENT: {
        auto&& t = evaluate_lvalue(expr.operand_, loc, true);
        const auto tmp = t;
        t.is_int() ? t = t + core::value::int_t{1} : t = t + core::value::double_t{1};
        return tmp;
      }
      case tt::DECREMENT: {
        auto&& t = evaluate_lvalue(expr.operand_, loc, true);
        const auto tmp = t;
        t.is_int() ? t = t - core::value::int_t{1} : t = t - core::value::double_t{1};
        return tmp;
      }
      default:
        raise_error(loc, err::unsupported_postfix_operator);
    }
  } catch (const core::value_error& e) {
    raise_error(loc, e.code_, op);
  }
}

core::value interpreter::evaluate(const ast::call_expr& expr) {
  const auto [_, name, loc] = expr.callee_;

  const auto func = registry_.find(name);
  if (func == registry_.end()) [[unlikely]]
    raise_error(loc, err::undefined_function, name);

  if (recursion_depth_ >= MAX_RECURSION_DEPTH) [[unlikely]]
    raise_error(loc, err::stack_overflow);
  recursion_depth_++;
  struct depth_guard {
    std::uint32_t& d;
    ~depth_guard() { d--; }
  } d_guard{recursion_depth_};

  auto args_buf = std::array<std::byte, 512>{};
  auto args_mr = std::pmr::monotonic_buffer_resource{args_buf.data(), args_buf.size()};
  auto args = std::pmr::vector<core::value>(&args_mr);
  args.reserve(expr.args_.size());
  std::ranges::transform(expr.args_, std::back_inserter(args), [&](auto&& a) { return evaluate(a); });

  try {
    return std::visit([&](const auto& ptr) { return call(ptr, args); }, func->info_);
  } catch (const core::value_error& e) {
    raise_error(loc, e.code_, name);
  }
}

core::value interpreter::evaluate(const ast::method_call_expr& expr) {
  const auto [_, name, loc] = expr.method_;

  if (is_lvalue(expr.object_)) {
    auto&& recv = evaluate_lvalue(expr.object_, loc, true);
    if (!recv.is_struct()) raise_error(loc, err::not_a_struct);

    recv.make_unique_struct();
    auto&& st = recv.to_struct();
    const auto struct_it = registry_.find(st.node_->name_);
    const auto st_ast = std::get<core::symbol_registry::struct_ptr>(struct_it->info_);
    const auto method_it = std::ranges::find(st_ast->methods_, name, [](auto&& m) { return m.name_.lexeme_; });
    if (method_it == st_ast->methods_.end()) [[unlikely]]
      raise_error(loc, err::no_such_method, st.node_->name_, name);

    auto args_buf = std::array<std::byte, 512>{};
    auto args_mr = std::pmr::monotonic_buffer_resource{args_buf.data(), args_buf.size()};
    auto args = std::pmr::vector<core::value>(&args_mr);
    args.reserve(expr.args_.size());
    std::ranges::transform(expr.args_, std::back_inserter(args), [&](auto&& a) { return evaluate(a); });

    const auto recv_idx = static_cast<std::size_t>(&recv - slots_.data());

    const auto saved_this = this_idx_;
    struct this_guard {
      std::size_t& r;
      std::size_t p;
      ~this_guard() { r = p; }
    } g{this_idx_, saved_this};
    this_idx_ = recv_idx;

    return call(&*method_it, args);
  }

  raise_error(loc, err::not_a_struct);
}

core::value interpreter::call(core::symbol_registry::builtin_func_ptr ptr, std::span<const core::value> args) {
  auto&& result = ptr->impl_(args);
  if (ret_enabled_debug_) [[unlikely]]
    writer_.print_return(ptr->name_, result);
  return result;
}

core::value interpreter::call(core::symbol_registry::func_ptr ptr, std::span<const core::value> args) {
  auto&& params = ptr->params_;
  if (args.size() != params.size()) [[unlikely]]
    raise_error(ptr->loc_, err::argument_count_mismatch, ptr->name_.lexeme_, params.size(), args.size());

  const auto saved_base = frame_base_;
  const auto saved_size = slots_.size();
  frame_base_ = saved_size;
  slots_.resize(frame_base_ + ptr->max_slots_);

  for (auto&& [pos, arg] : std::views::enumerate(args)) slots_[frame_base_ + pos] = arg;

  struct frame_guard {
    std::vector<core::value>& slots;
    std::size_t& frame_base;
    std::size_t saved_base, saved_size;
    ~frame_guard() {
      slots.resize(saved_size);
      frame_base = saved_base;
    }
  } f{slots_, frame_base_, saved_base, saved_size};

  auto result = core::value{};
  for (auto&& s : ptr->body_) {
    auto&& r = execute(s);
    if (r.is_return()) {
      result = std::move(r.value_);
      break;
    }
  }

  if (ret_enabled_debug_) [[unlikely]]
    writer_.print_return(ptr->name_.lexeme_, result);
  return result;
}

core::value interpreter::call(core::symbol_registry::struct_ptr ptr, std::span<const core::value> args) {
  if (!args.empty()) [[unlikely]]
    raise_error(ptr->loc_, err::argument_count_mismatch, ptr->name_, 0, args.size());
  return core::value::struct_t{ptr, std::vector<core::value>(ptr->field_names_.size())};
}

}  // namespace cint
