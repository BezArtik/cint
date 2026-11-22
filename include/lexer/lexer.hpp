// lexer/lexer.hpp
#pragma once
#include "core/error/diagnostic.hpp"
#include "core/memory/arena.hpp"
#include "core/token/token.hpp"

#include <string_view>
#include <vector>

namespace cint {

class lexer {
 public:
  using token_list = core::arena_vector<core::token>;

  lexer(std::string_view source, core::diagnostic_sink& sink, core::arena& arena)
      : source_{source}, sink_{sink}, arena_{arena}, tokens_{arena_} {}

  [[nodiscard]] token_list scan_tokens();

 private:
  bool is_at_end() const noexcept;
  char advance() noexcept;
  char peek() const noexcept;
  char peek_next() const noexcept;
  bool match(char expected) noexcept;
  void scan_token();
  void add_token(core::token_type type);
  void consume_string();
  void consume_number();
  void consume_identifier_or_keyword();

  std::string_view source_;
  core::diagnostic_sink& sink_;
  core::arena& arena_;
  token_list tokens_;

  std::uint32_t start_{};
  std::uint32_t current_{};
  core::location loc_{1, 1};
  core::location start_loc_{1, 1};
};

}  // namespace cint
