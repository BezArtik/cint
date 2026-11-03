// tests/pipeline_harness.cpp

#include "pipeline_harness.hpp"

#include "core/symbol/symbol_registry.hpp"
#include "debug/debug_writer.hpp"
#include "parser/parser.hpp"
#include "runtime/interpreter.hpp"
#include "semantics/type_check.hpp"

#include <iostream>

namespace cint::tests {

bool pipeline_harness::lex() {
    auto lex = lexer{source_code_, reporter_, arena_};
    tokens_ = lex.scan_tokens();
    return !reporter_.has_error();
}

bool pipeline_harness::parse() {
    auto p = parser{tokens_, reporter_, arena_};
    ast_ = p.parse();
    if (!reporter_.has_error()) registry_.emplace(core::symbol_registry::build(ast_, reporter_));
    return !reporter_.has_error();
}

bool pipeline_harness::check_semantics() {
    auto checker = type_checker{reporter_, *registry_};
    return checker.check(ast_) && !reporter_.has_error();
}

bool pipeline_harness::interpret() {
    auto writer = debug::debug_writer{std::cerr, debug::trace_level::none};
    auto interp = interpreter{reporter_, *registry_, writer};
    interp.interpret(ast_);
    return !reporter_.has_error();
}

}  // namespace cint::tests
