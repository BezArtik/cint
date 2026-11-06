// tests/pipeline_harness.cpp

#include "pipeline_harness.hpp"

#include "core/symbol/symbol_registry.hpp"
#include "debug/debug_writer.hpp"
#include "parser/parser.hpp"
#include "runtime/interpreter.hpp"
#include "semantics/type_check.hpp"

namespace cint::tests {

bool pipeline_harness::lex() {
    auto lex = lexer{source_code_, sink_, arena_};
    tokens_ = lex.scan_tokens();
    return !sink_.has_errors();
}

bool pipeline_harness::parse() {
    auto p = parser{tokens_, sink_, arena_};
    ast_ = p.parse();
    registry_.emplace(core::symbol_registry::build(ast_, sink_));
    return !sink_.has_errors();
}

bool pipeline_harness::check_semantics() {
    auto checker = type_checker{sink_, *registry_};
    return checker.check(ast_) && !sink_.has_errors();
}

bool pipeline_harness::interpret() {
    auto writer = debug::debug_writer{debug::trace_level::none};
    auto interp = interpreter{sink_, *registry_, writer};
    interp.interpret(ast_);
    return !sink_.has_errors();
}

}  // namespace cint::tests
