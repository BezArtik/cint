// tests/pipeline_harness.cpp

#include "pipeline_harness.hpp"

#include "core/symbol/symbol_registry.hpp"
#include "debug/debug_writer.hpp"
#include "parser/parser.hpp"
#include "runtime/interpreter.hpp"

namespace cint::tests {

void pipeline_harness::lex() {
  if (lexed_) return;
  auto lex = lexer{source_code_, sink_, arena_};
  tokens_ = lex.scan_tokens();
  lexed_ = true;
}

void pipeline_harness::parse() {
  lex();
  if (parsed_) return;

  auto p = parser{tokens_, sink_, arena_, literals_};
  ast_ = p.parse();
  parsed_ = true;
}

void pipeline_harness::interpret() {
  parse();
  if (sink_.has_errors()) return;
  const auto registry = core::symbol_registry{ast_, sink_};
  auto writer = debug::debug_writer{debug::trace_level::none};
  try {
    const auto interp = interpreter{ast_, registry, writer, literals_};
  } catch (const runtime_error& e) {
    sink_.report_formatted(e.loc_, e.code_, e.message_);
  }
}

bool pipeline_harness::run_all() {
  lex();
  if (had_error()) return false;
  parse();
  if (had_error()) return false;
  interpret();
  return !had_error();
}

}  // namespace cint::tests
