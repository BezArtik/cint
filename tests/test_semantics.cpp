// test_semantics.cpp

#include "core/error/error_report.hpp"
#include "core/utils/arena.hpp"
#include "core/utils/builtins.hpp"
#include "core/utils/function_registry.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantics/type_check.hpp"

#include <gtest/gtest.h>
#include <string>

namespace tests {

class semantics_harness {
public:
    semantics_harness(std::string source) : source_code_(std::move(source)), reporter_(source_code_), mr_(arena_) {
        lexer lex(source_code_, reporter_, mr_);
        tokens_ = lex.scan_tokens();
        parser p(tokens_, reporter_, arena_, mr_);
        ast_ = p.parse();
        auto registry = core::function_registry::build(ast_, core::builtins);
        semantics::type_checker checker(reporter_, registry);
        check_ok_ = checker.check(ast_);
        had_error_ = reporter_.has_error();
    }

    bool ok() const { return check_ok_ && !had_error_; }
    bool had_error() const { return had_error_; }

private:
    std::string source_code_;
    core::error_reporter reporter_;
    core::arena arena_;
    core::arena_memory_resource mr_;
    lexer::token_list tokens_;
    parser::ast_list ast_;
    bool check_ok_ = false;
    bool had_error_ = false;
};

struct valid_program_case {
    std::string_view source_;
    std::string_view description_;
};

class valid_program_test : public ::testing::TestWithParam<valid_program_case> {};

TEST_P(valid_program_test, type_checks) {
    const auto& tc = GetParam();
    semantics_harness h(std::string{tc.source_});
    EXPECT_TRUE(h.ok()) << "Unexpected error for: " << tc.description_;
}
// clang-format off
INSTANTIATE_TEST_SUITE_P(
    basics, valid_program_test,
    ::testing::Values(valid_program_case{"void foo() { }", "empty void func"},
                      valid_program_case{"int foo(int a) { return a; }", "param return"},
                      valid_program_case{"int foo() { int x = 42; return x; }", "var init"},
                      valid_program_case{"double foo() { int x = 1; return x; }", "int to double"},
                      valid_program_case{"int foo() { int x; x = 5; return x; }", "assign"},
                      valid_program_case{"int foo() { int x = 1; x += 2; return x; }", "compound assign"},
                      valid_program_case{"int foo() { if (true) { return 1; } return 0; }", "if true"},
                      valid_program_case{"int foo() { if (true) { return 1; } else { return 2; } }", "if else"},
                      valid_program_case{"int foo() { while (true) { } return 0; }", "while"},
                      valid_program_case{"int foo() { for (int i=0; i<10; i=i+1) { } return 0; }", "for"},
                      valid_program_case{"int foo(int a, int b) { return a + b; } foo(1,2);", "call"},
                      valid_program_case{"bool foo() { return true && false; }", "logical and"},
                      valid_program_case{"bool foo() { return true || false; }", "logical or"},
                      valid_program_case{"bool foo() { return !true; }", "not"},
                      valid_program_case{"int x = 0; ++x;", "prefix inc"},
                      valid_program_case{"int x = 0; x++;", "postfix inc"},
                      valid_program_case{"int arr[] = {1, 2, 3}; int x = arr[0] + arr[1] + arr[2];", "array access"}));
// clang-format on
struct type_error_case {
    std::string_view source_;
    std::string_view description_;
};

class type_error_test : public ::testing::TestWithParam<type_error_case> {};

TEST_P(type_error_test, reports_error) {
    const auto& tc = GetParam();
    semantics_harness h(std::string{tc.source_});
    EXPECT_TRUE(h.had_error()) << "Expected error for: " << tc.description_;
}
// clang-format off
INSTANTIATE_TEST_SUITE_P(
    all, type_error_test,
    ::testing::Values(type_error_case{"int x = true;", "bool to int assign"},
                      type_error_case{"int foo() { return x; }", "undefined var"},
                      type_error_case{"if (42) { }", "non-bool condition"},
                      type_error_case{"int x; int x;", "redeclaration"},
                      type_error_case{"return 0;", "return outside func"},
                      type_error_case{"int foo() { return true; }", "return type mismatch"},
                      type_error_case{"int foo() { return; }", "missing return value"},
                      type_error_case{"void foo() { return 42; }", "return value in void"},
                      type_error_case{"int foo() { 42 = 0; return 0; }", "assign to literal"},
                      type_error_case{"int bar() { return foo(); }", "undeclared func"},
                      type_error_case{"int foo(int a) { return a; } int bar() { return foo(1,2); }", "arg count"},
                      type_error_case{"int foo(bool b) { return 0; } int bar() { return foo(42); }", "arg type"},
                      type_error_case{"bool b = !42;", "not on int"}, 
                      type_error_case{"int x; ++true;", "inc on bool"},
                      type_error_case{"bool x = true + false;", "arithmetic on bool"}));
// clang-format on
}  // namespace tests
