// tests/test_semantics.cpp

#include "pipeline_harness.hpp"

#include <gtest/gtest.h>
#include <string_view>

namespace tests {

struct type_error_case {
    std::string_view source_;
    std::string_view description_;
};

class type_error_test : public ::testing::TestWithParam<type_error_case> {};

TEST_P(type_error_test, reports_error) {
    auto&& tc = GetParam();
    pipeline_harness h(tc.source_);
    h.lex();
    h.parse();
    h.check_semantics();
    EXPECT_TRUE(h.had_error()) << "Expected error for: " << tc.description_;
}
// clang-format off
INSTANTIATE_TEST_SUITE_P(
    all, type_error_test,
    ::testing::Values(
        type_error_case{"int x = true;", "bool to int assign"},
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
        type_error_case{"bool x = true + false;", "arithmetic on bool"},
        type_error_case{"struct Point { int x; int x; };", "duplicate field"},
        type_error_case{"struct Point { int x; int y; }; void foo() { struct Point p; p.z = 10; }", "access unknown field"},
        type_error_case{"void foo() { int x; x.y = 10; }", "dot on non-struct"}
));
// clang-format on

}  // namespace tests
