// tests/pipeline_harness.cpp

#include "pipeline_harness.hpp"

#include "core/utils/symbol_registry.hpp"
#include "runtime/interpreter.hpp"
#include "semantics/type_check.hpp"

namespace tests {

pipeline_harness::pipeline_harness(std::string_view source)
    : source_code_(source), reporter_(source_code_), mr_(arena_) {}

bool pipeline_harness::lex() {
    lexer lex(source_code_, reporter_, mr_);
    tokens_ = lex.scan_tokens();
    return !reporter_.has_error();
}

bool pipeline_harness::parse() {
    parser p(tokens_, reporter_, arena_, mr_);
    ast_ = p.parse();
    if (!reporter_.has_error()) registry_.emplace(core::symbol_registry::build(ast_, core::builtins));
    return !reporter_.has_error();
}

bool pipeline_harness::check_semantics() {
    semantics::type_checker checker(reporter_, *registry_);
    return checker.check(ast_) && !reporter_.has_error();
}

bool pipeline_harness::interpret() {
    runtime::interpreter interp(reporter_, *registry_);
    interp.interpret(ast_);
    return !reporter_.has_error();
}

bool pipeline_harness::run_all() {
    return lex() && parse() && check_semantics() && interpret();
}

const lexer::token_list& pipeline_harness::tokens() const noexcept {
    return tokens_;
}
const parser::ast_list& pipeline_harness::ast() const noexcept {
    return ast_;
}
bool pipeline_harness::had_error() const noexcept {
    return reporter_.has_error();
}

}  // namespace tests
