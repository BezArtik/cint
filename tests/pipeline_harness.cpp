// tests/pipeline_harness.cpp

#include "pipeline_harness.hpp"

#include "core/symbol/symbol_registry.hpp"
#include "parser/parser.hpp"
#include "runtime/interpreter.hpp"
#include "semantics/type_check.hpp"

namespace tests {

bool pipeline_harness::lex() {
    lexer lex{source_code_, reporter_, mr_};
    tokens_ = lex.scan_tokens();
    return !reporter_.has_error();
}

bool pipeline_harness::parse() {
    parser p{tokens_, reporter_, arena_, mr_};
    ast_ = p.parse();
    if (!reporter_.has_error()) registry_.emplace(core::symbol_registry::build(ast_));
    return !reporter_.has_error();
}

bool pipeline_harness::check_semantics() {
    type_checker checker{reporter_, *registry_};
    return checker.check(ast_) && !reporter_.has_error();
}

bool pipeline_harness::interpret() {
    interpreter interp{reporter_, *registry_};
    interp.interpret(ast_);
    return !reporter_.has_error();
}

}  // namespace tests
