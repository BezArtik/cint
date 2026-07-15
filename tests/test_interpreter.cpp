// test_interpreter.cpp

#include "core/error/error_report.hpp"
#include "core/utils/arena.hpp"
#include "core/utils/builtins.hpp"
#include "core/utils/function_registry.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "runtime/interpreter.hpp"
#include "semantics/type_check.hpp"

#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace tests {

class interpreter_harness {
public:
    interpreter_harness(std::string source) : source_code_(std::move(source)), reporter_(source_code_), mr_(arena_) {
        lexer::lexer lex(source_code_, reporter_, mr_);
        tokens_ = lex.scan_tokens();
        parser::parser p(tokens_, reporter_, arena_, mr_);
        ast_ = p.parse();
        auto registry = core::function_registry::build(ast_, core::builtins);
        semantics::type_checker checker(reporter_, registry);
        checker.check(ast_);
        runtime::interpreter interp(reporter_, registry);
        interp.interpret(ast_);
        ok_ = !reporter_.has_error();
    }

    bool ok() const noexcept { return ok_; }

private:
    std::string source_code_;
    core::error_reporter reporter_;
    core::arena arena_;
    core::arena_memory_resource mr_;
    std::pmr::vector<core::token> tokens_;
    std::vector<ast::stmt_ptr> ast_;
    bool ok_ = false;
};

struct valid_execution_case {
    std::string_view source_;
    std::string_view description_;
};

class valid_execution_test : public ::testing::TestWithParam<valid_execution_case> {};

TEST_P(valid_execution_test, runs_without_error) {
    const auto& tc = GetParam();
    interpreter_harness h(std::string{tc.source_});
    EXPECT_TRUE(h.ok()) << "Unexpected runtime error for: " << tc.description_;
}
// clang-format off
INSTANTIATE_TEST_SUITE_P(
    all, valid_execution_test,
    ::testing::Values(valid_execution_case{"int x = 42;", "var decl"}, 
                      valid_execution_case{"int x = 1 + 2;", "add"},
                      valid_execution_case{"int x = 10 - 3;", "sub"}, 
                      valid_execution_case{"int x = 4 * 5;", "mul"},
                      valid_execution_case{"int x = 20 / 4;", "div"}, 
                      valid_execution_case{"int x = 17 % 5;", "mod"},
                      valid_execution_case{"double x = 3.14;", "double var"},
                      valid_execution_case{"bool b = true;", "bool var"},
                      valid_execution_case{"string s = \"hi\";", "string var"},
                      valid_execution_case{"if (true) { }", "if true"},
                      valid_execution_case{"if (true) { } else { }", "if else"},
                      valid_execution_case{"int x = 0; while (x < 3) { x = x + 1; }", "while"},
                      valid_execution_case{"int s = 0; for (int i=0; i<5; i=i+1) { s = s+i; }", "for"},
                      valid_execution_case{"int x = 0; x = 42;", "assign"},
                      valid_execution_case{"int x = 10; x += 5;", "compound assign"},
                      valid_execution_case{"int x = 0; ++x;", "prefix inc"},
                      valid_execution_case{"int x = 0; x++;", "postfix inc"},
                      valid_execution_case{"int add(int a,int b) { return a+b; }", "func decl"},
                      valid_execution_case{"int f() { return 42; } f();", "func call"},
                      valid_execution_case{"bool f() { return true && false; }", "and"},
                      valid_execution_case{"bool f() { return true || false; }", "or"},
                      valid_execution_case{"bool f() { return !true; }", "not"}));
// clang-format on

struct runtime_error_case {
    std::string_view source_;
    std::string_view description_;
};

class runtime_error_test : public ::testing::TestWithParam<runtime_error_case> {};

TEST_P(runtime_error_test, reports_error) {
    const auto& tc = GetParam();
    interpreter_harness h(std::string{tc.source_});
    EXPECT_FALSE(h.ok()) << "Expected runtime error for: " << tc.description_;
}

INSTANTIATE_TEST_SUITE_P(all, runtime_error_test,
                         ::testing::Values(runtime_error_case{"int x = 1/0;", "div by zero int"},
                                           runtime_error_case{"double x = 1.0/0.0;", "div by zero double"},
                                           runtime_error_case{"int x = 1%0;", "mod by zero"},
                                           runtime_error_case{"int f() { return f(); } int x = f();",
                                                              "stack overflow"}));

}  // namespace tests
