// tests/test_semantics.cpp

#include "core/error/error_codes.hpp"
#include "pipeline_harness.hpp"

#include <gtest/gtest.h>
#include <string_view>

namespace cint::tests {

using err = core::error_code;

struct type_error_case {
    std::string_view source_;
    err expected_code_;
    std::string_view description_;
};

class type_error_test : public ::testing::TestWithParam<type_error_case> {};

TEST_P(type_error_test, reports_error) {
    auto&& tc = GetParam();
    auto h = pipeline_harness{tc.source_};
    h.lex();
    h.parse();
    h.check_semantics();
    if (!h.had_error(tc.expected_code_)) h.render_diagnostics();
    EXPECT_TRUE(h.had_error(tc.expected_code_)) << "for: " << tc.description_;
}
// clang-format off
INSTANTIATE_TEST_SUITE_P(
    all, type_error_test,
    ::testing::Values(
        type_error_case{"int x = true;", err::type_mismatch_initialization, "bool to int assign"},
        type_error_case{"int foo() { return x; }", err::undefined_variable, "undefined var"},
        type_error_case{"if (42) { }", err::condition_not_bool, "non-bool condition"},
        type_error_case{"int x; int x;", err::redeclaration, "redeclaration"},
        type_error_case{"return 0;", err::return_outside_function, "return outside func"},
        type_error_case{"int foo() { return true; }", err::return_type_mismatch, "return type mismatch"},
        type_error_case{"int foo() { return; }", err::return_missing_value, "missing return value"},
        type_error_case{"void foo() { return 42; }", err::return_type_mismatch, "return value in void"},
        type_error_case{"int foo() { 42 = 0; return 0; }", err::type_mismatch_assignment, "assign to literal"},
        type_error_case{"int bar() { return foo(); }", err::undefined_function, "undeclared func"},
        type_error_case{"int foo(int a) { return a; } int bar() { return foo(1,2); }",
                        err::argument_count_mismatch, "arg count"},
        type_error_case{"int foo(bool b) { return 0; } int bar() { return foo(42); }",
                        err::argument_type_mismatch, "arg type"},
        type_error_case{"bool b = !42;", err::not_requires_bool, "not on int"},
        type_error_case{"int x; ++true;", err::increment_requires_numeric, "inc on bool"},
        type_error_case{"bool x = true + false;", err::arithmetic_requires_numeric, "arithmetic on bool"},
        type_error_case{"struct Point { int x; int x; };", err::redeclaration, "duplicate field"},
        type_error_case{"struct Point { int x; int y; }; void foo() { struct Point p; p.z = 10; }",
                        err::no_such_field, "access unknown field"},
        type_error_case{"void foo() { int x; x.y = 10; }", err::not_a_struct, "dot on non-struct"},
        type_error_case{"{ int x; int x; }}", err::redeclaration, "rederlaration in block"},
        type_error_case{"while(1) {}", err::condition_not_bool, "while condition not bool"},
        type_error_case{"for (; 1; ) {}", err::condition_not_bool, "for condition not bool"}, 
        type_error_case{"struct U arr[3];", core::error_code::undefined_type, "undefined struct in array"},
        type_error_case{"void arr[3];", core::error_code::void_variable, "void array"},
        type_error_case{"struct S { void x; };", core::error_code::void_variable, "void field"},
        type_error_case{"int f(void x) { return 0; }", core::error_code::void_variable, "void param"}
));
// clang-format on

TEST(semantics_stress, struct_init_mixed_field_types_is_ok) {
    auto h = pipeline_harness{
        "struct P { int x; double y; }; "
        "struct P p = {1, 2.0};"};
    h.lex();
    h.parse();
    h.check_semantics();
    if (h.had_error()) h.render_diagnostics();
    EXPECT_FALSE(h.had_error());
}

TEST(semantics_stress, bool_equality_is_ok) {
    auto h = pipeline_harness{"bool a = true; bool b = false; bool c = a == b;"};
    h.lex();
    h.parse();
    h.check_semantics();
    if (h.had_error()) h.render_diagnostics();
    EXPECT_FALSE(h.had_error());
}

TEST(semantics_stress, struct_after_failed_init_still_defined) {
    auto h = pipeline_harness{
        "struct P { int x; double y; }; "
        "struct P p = {1, 2.0}; "
        "double y = p.y;"};
    h.lex();
    h.parse();
    h.check_semantics();
    if (h.had_error()) h.render_diagnostics();
    EXPECT_FALSE(h.had_error());
}

}  // namespace cint::tests
