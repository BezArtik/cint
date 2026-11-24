// tests/test_interpreter.cpp

#include "core/error/error_codes.hpp"
#include "pipeline_harness.hpp"

#include <gtest/gtest.h>
#include <string_view>

namespace cint::tests {

using err = core::error_code;

struct valid_execution_case {
  std::string_view source_;
  std::string_view description_;
};

struct valid_execution_test : ::testing::TestWithParam<valid_execution_case> {};

TEST_P(valid_execution_test, runs_without_error) {
  auto&& tc = GetParam();
  auto h = pipeline_harness{tc.source_};
  h.run_all();
  if (h.had_error()) h.render_diagnostics();
  EXPECT_FALSE(h.had_error()) << "Unexpected error for: " << tc.description_;
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    all, valid_execution_test,
    ::testing::Values(
        valid_execution_case{"let x = 42;", "var decl"},
        valid_execution_case{"let x = 1 + 2;", "add"},
        valid_execution_case{"let x = 10 - 3;", "sub"},
        valid_execution_case{"let x = 4 * 5;", "mul"},
        valid_execution_case{"let x = 20 / 4;", "div"},
        valid_execution_case{"let x = 17 % 5;", "mod"},
        valid_execution_case{"let x = 3.14;", "double var"},
        valid_execution_case{"let b = true;", "bool var"},
        valid_execution_case{"let s = \"hi\";", "string var"},
        valid_execution_case{"if true { }", "if true"},
        valid_execution_case{"if true { } else { }", "if else"},
        valid_execution_case{"let x = 0; while x < 3 { x = x + 1; }", "while"},
        valid_execution_case{"let s = 0; for (let i = 0; i < 5; i = i + 1) { s = s + i; }", "for"},
        valid_execution_case{"let x = 0; x = 42;", "assign"},
        valid_execution_case{"let x = 10; x += 5;", "compound assign"},
        valid_execution_case{"let x = 0; ++x;", "prefix inc"},
        valid_execution_case{"let x = 0; x++;", "postfix inc"},
        valid_execution_case{"fn add(a, b) { return a + b; } add(1, 2);", "func decl + call"},
        valid_execution_case{"fn f() { return 42; } f();", "func call"},
        valid_execution_case{"fn f() { return true && false; } f();", "and"},
        valid_execution_case{"fn f() { return true || false; } f();", "or"},
        valid_execution_case{"fn f() { return !true; } f();", "not"},
        valid_execution_case{"fn foo() { } foo();", "empty void func"},
        valid_execution_case{"fn foo(a) { return a; } foo(42);", "param return"},
        valid_execution_case{"fn foo() { let x = 42; return x; } foo();", "var init + return"},
        valid_execution_case{"struct Point { x, y }", "struct decl"},
        valid_execution_case{
            "struct Point { x, y } "
            "fn foo(p) { p.x = 10; } "
            "let p = Point(); foo(p);",
            "struct param access"},
        valid_execution_case{
            "struct Point { x, y } "
            "let p = Point(); p.x = 42; p.y = 99;",
            "struct var assign"},
        valid_execution_case{
            "struct Point { x, y } "
            "fn foo() { let p = Point(); p.x = 10; p.y = 20; } foo();",
            "struct in func"},
        valid_execution_case{
            "struct Point { x, y } "
            "fn make_point(x, y) { "
            "  let p = Point(); p.x = x; p.y = y; return p; "
            "} "
            "let p = make_point(10, 20);",
            "return struct from func"},
        valid_execution_case{
            "struct Person { name, age } "
            "let p = Person(); p.name = \"Alice\"; p.age = 30;",
            "struct with string field"},
        valid_execution_case{"let x = 1; { let y = 2; let z = x + y; }", "block with shadowing"},
        valid_execution_case{
            "fn f(a) { if a > 0 { return a; } return -a; } "
            "let x = f(5) + f(-5);",
            "nested if + unary minus"},
        valid_execution_case{
            "struct Point { x, y } "
            "let a = Point(); a.x = 1; a.y = 2; "
            "let b = Point(); b.x = 1; b.y = 2; "
            "if !(a == b) { undefined_fn(); }",
            "struct equality"},
        valid_execution_case{
            "let s = \"ab\" + \"cd\"; let t = \"ab\" * 3;", "string concat/repetition"},
        valid_execution_case{"let x = 0; if \"hello\" { x = 1; }", "truthy string in if"},
        valid_execution_case{"let x = 42; if x { x = 0; }", "truthy int in if"},
        valid_execution_case{
            "struct Point { x, y, "
            "  fn distance(q) { "
            "    let dx = this.x - q.x; "
            "    let dy = this.y - q.y; "
            "    return sqrt(dx*dx + dy*dy); "
            "  } "
            "} "
            "let a = Point(); a.x = 0; a.y = 0; "
            "let b = Point(); b.x = 3; b.y = 4; "
            "let d = a.distance(b); "
            "if d < 4.9 { undefined_fn(); } "
            "if d > 5.1 { undefined_fn(); }",
            "distance via method"},
        valid_execution_case{
            "struct Outer { inner, "
            "  fn double_x() { this.inner.x = this.inner.x * 2; } "
            "} "
            "struct Inner { x } "
            "let o = Outer(); o.inner = Inner(); o.inner.x = 5; "
            "o.double_x(); "
            "if o.inner.x != 10 { undefined_fn(); }",
            "nested struct mutation via method"},
        valid_execution_case{
            "struct Counter { value, fn inc() { this.value = this.value + 1; } } "
            "fn run(c) { c.inc(); c.inc(); } "
            "let c = Counter(); c.value = 0; "
            "run(c); "
            "if c.value != 0 { undefined_fn(); }",  
            "method mutation on copy doesn't affect original"},
        valid_execution_case{
            "struct R { n } "
            "fn f() { let r = R(); r.n = 7; let copy = r; copy.n = 99; "
            "  if r.n != 7 { undefined_fn(); } } f();",
            "struct copy is independent"}
));

struct runtime_error_case {
    std::string_view source_;
    err expected_code_;
    std::string_view description_;
};

struct runtime_error_test : ::testing::TestWithParam<runtime_error_case> {};

TEST_P(runtime_error_test, reports_error) {
    auto&& tc = GetParam();
    auto h = pipeline_harness{tc.source_};
    h.run_all();
    if (!h.had_error(tc.expected_code_)) h.render_diagnostics();
    EXPECT_TRUE(h.had_error(tc.expected_code_)) << "for: " << tc.description_;
}

INSTANTIATE_TEST_SUITE_P(
    all, runtime_error_test,
    ::testing::Values(
        runtime_error_case{"let x = 1 / 0;", err::division_by_zero, "div by zero int"},
        runtime_error_case{"let x = 1.0 / 0.0;", err::division_by_zero, "div by zero double"},
        runtime_error_case{"fn f() { return f(); } f();", err::stack_overflow, "stack overflow"},
        runtime_error_case{"undefined_fn();", err::undefined_function, "undefined function"},
        runtime_error_case{"let x = 42; x.nope = 1;", err::not_a_struct, "dot on non-struct"},
        runtime_error_case{"fn f() { } f(1);", err::argument_count_mismatch, "arg count"},
        runtime_error_case{"let y;", err::expected_equal, "expected equal"},
        runtime_error_case{"let x = \"s\"; let y = x + 1;", err::invalid_conversion, "string + int"},
        runtime_error_case{"let s = \"a\" - \"b\";", err::invalid_conversion, "string minus"},
        runtime_error_case{"struct R { x } let r = R(); r.nope();",
                           err::no_such_method, "method not found"},
        runtime_error_case{"let x = 42; x.foo();",
                           err::not_a_struct, "method on non-struct"}
));
// clang-format on

TEST(interpreter_stress, short_circuit_and_skips_rhs) {
  auto h = pipeline_harness{
      "fn zero() { return 0; } "
      "fn f() { return false && (1 / zero() == 0); } "
      "f();"};
  h.run_all();
  if (h.had_error()) h.render_diagnostics();
  EXPECT_FALSE(h.had_error());
}

TEST(interpreter_stress, short_circuit_or_skips_rhs) {
  auto h = pipeline_harness{
      "fn zero() { return 0; } "
      "fn f() { return true || (1 / zero() == 0); } "
      "f();"};
  h.run_all();
  if (h.had_error()) h.render_diagnostics();
  EXPECT_FALSE(h.had_error());
}

TEST(interpreter_stress, return_from_nested_if) {
  auto h = pipeline_harness{"fn f() { if true { if true { return 42; } } return 0; } f();"};
  h.run_all();
  if (h.had_error()) h.render_diagnostics();
  EXPECT_FALSE(h.had_error());
}

TEST(interpreter_stress, return_from_while) {
  auto h = pipeline_harness{"fn f() { let i = 0; while true { if i > 3 { return i; } ++i; } } f();"};
  h.run_all();
  if (h.had_error()) h.render_diagnostics();
  EXPECT_FALSE(h.had_error());
}

TEST(interpreter_stress, bool_equality_runtime) {
  auto h = pipeline_harness{
      "let a = true; let b = false; "
      "let c = a == b; let d = a == a;"};
  h.run_all();
  if (h.had_error()) h.render_diagnostics();
  EXPECT_FALSE(h.had_error());
}

TEST(interpreter_stress, struct_field_write_then_read) {
  auto h = pipeline_harness{
      "struct Point { x, y } "
      "let p = Point(); "
      "p.x = 10; p.y = 20; "
      "if p.x != 10 { undefined_fn(); } "
      "if p.y != 20 { undefined_fn(); }"};
  h.run_all();
  if (h.had_error()) h.render_diagnostics();
  EXPECT_FALSE(h.had_error());
}

}  // namespace cint::tests
