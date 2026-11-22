// tests/pipeline_harness.hpp

#pragma once

#include "ast/node.hpp"
#include "core/error/diagnostic.hpp"
#include "core/error/diagnostic_renderer.hpp"
#include "core/error/error_codes.hpp"
#include "core/memory/arena.hpp"
#include "core/utils/literal_pool.hpp"
#include "lexer/lexer.hpp"

#include <string>
#include <string_view>

namespace cint::tests {

class pipeline_harness {
 public:
  pipeline_harness(std::string_view source)
      : source_code_{source}, renderer_{source_code_}, tokens_{arena_}, ast_{arena_} {}

  void lex();
  void parse();
  void interpret();
  bool run_all();

  const lexer::token_list& tokens() const noexcept { return tokens_; }
  const ast::stmt_list& ast() const noexcept { return ast_; }
  const core::literal_pool& literals() const noexcept { return literals_; }
  bool had_error() const noexcept { return sink_.has_errors(); }
  bool had_error(core::error_code code) const noexcept { return sink_.has_error(code); }
  void render_diagnostics() { renderer_.render(sink_.begin(), sink_.end()); }

 private:
  std::string source_code_;
  core::diagnostic_sink sink_;
  core::diagnostic_renderer renderer_;
  core::arena arena_;
  core::literal_pool literals_;
  lexer::token_list tokens_;
  ast::stmt_list ast_;
  bool lexed_{};
  bool parsed_{};
};

}  // namespace cint::tests
