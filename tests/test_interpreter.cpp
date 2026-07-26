// tests/test_interpreter.cpp

#include "pipeline_harness.hpp"

#include <gtest/gtest.h>
#include <string_view>

namespace tests {

struct valid_execution_case {
    std::string_view source_;
    std::string_view description_;
};

class valid_execution_test : public ::testing::TestWithParam<valid_execution_case> {};

TEST_P(valid_execution_test, runs_without_error) {
    const auto& tc = GetParam();
    pipeline_harness h(tc.source_);
    EXPECT_TRUE(h.run_all()) << "Unexpected runtime error for: " << tc.description_;
}
// clang-format off
INSTANTIATE_TEST_SUITE_P(
    all, valid_execution_test,
    ::testing::Values(
        valid_execution_case{"int x = 42;", "var decl"},
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
        valid_execution_case{"bool f() { return !true; }", "not"},
        valid_execution_case{"void foo() { }", "empty void func"},
        valid_execution_case{"int foo(int a) { return a; }", "param return"},
        valid_execution_case{"int foo() { int x = 42; return x; }", "var init + return"},
        valid_execution_case{"double foo() { int x = 1; return x; }", "int to double"},
        valid_execution_case{"int arr[] = {1, 2, 3}; int x = arr[0] + arr[1] + arr[2];", "array access"},
        valid_execution_case{"struct Point { int x; int y; };", "struct decl"},
        valid_execution_case{
            "struct Point { int x; int y; }; "
            "void foo(struct Point p) { p.x = 10; } ", 
            "struct param access"},
        valid_execution_case{
            "struct Point { int x; int y; }; "
            "void print_point(struct Point p) { } "
            "void foo() { struct Point p; p.x = 0; p.y = 0; print_point(p); }",
            "pass struct to func"},
        valid_execution_case{
            "struct Point { int x; int y; }; "
            "struct Point p; p.x = 42; p.y = 99;", 
            "struct var assign"},
        valid_execution_case{
            "struct Point { int x; int y; }; "
            "void foo() { struct Point p; p.x = 10; p.y = 20; } foo();",
            "struct in func"},
        valid_execution_case{
            "struct Point { int x; int y; }; "
            "struct Rect { struct Point tl; struct Point br; }; "
            "struct Rect r; "
            "r.tl.x = 1; r.tl.y = 2; r.br.x = 3; r.br.y = 4; ", 
            "nested struct assign"},
        valid_execution_case{
            "struct Point { int x; int y; }; "
            "struct Point make_point(int x, int y) { struct Point p; p.x = x; p.y = y; return p; } "
            "struct Point p = make_point(10, 20); ", 
            "return struct from func"},
        valid_execution_case{
            "struct Point { int x; int y; }; "
            "void foo() { struct Point p1; p1.x = 1; p1.y = 2; struct Point p2; p2 = p1; } foo(); ",
            "struct copy"},
        valid_execution_case{
            "struct Person { string name; int age; }; "
            "struct Person p; p.name = \"Alice\"; p.age = 30; ",
            "struct with string field"}
));

struct runtime_error_case {
    std::string_view source_;
    std::string_view description_;
};

class runtime_error_test : public ::testing::TestWithParam<runtime_error_case> {};

TEST_P(runtime_error_test, reports_error) {
    const auto& tc = GetParam();
    pipeline_harness h(tc.source_);
    h.lex();
    h.parse();
    h.check_semantics();
    h.interpret();
    EXPECT_TRUE(h.had_error()) << "Expected runtime error for: " << tc.description_;
}

INSTANTIATE_TEST_SUITE_P(
    all, runtime_error_test,
    ::testing::Values(
        runtime_error_case{"int x = 1/0;", "div by zero int"},
        runtime_error_case{"double x = 1.0/0.0;", "div by zero double"},
        runtime_error_case{"int x = 1%0;", "mod by zero"},
        runtime_error_case{"int f() { return f(); } int x = f();", "stack overflow"}
));
// clang-format on
}  // namespace tests
