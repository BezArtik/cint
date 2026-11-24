// tests/test_parser.cpp

#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/error/error_codes.hpp"
#include "pipeline_harness.hpp"

#include <gtest/gtest.h>
#include <stdexcept>
#include <string_view>
#include <typeinfo>

namespace cint::tests {

namespace {

template <typename T>
const T* as(const ast::statement& s) noexcept {
  const auto p = std::get_if<core::arena_ptr<T>>(&s);
  return p ? p->get() : nullptr;
}

template <typename T>
const T* as(const ast::expression& e) noexcept {
  const auto p = std::get_if<core::arena_ptr<T>>(&e);
  return p ? p->get() : nullptr;
}

template <typename T>
const T* must(const ast::statement& s) {
  const auto p = as<T>(s);
  if (!p) {
    ADD_FAILURE() << "expected " << typeid(T).name() << " (statement)";
    throw std::runtime_error{"must<T>: wrong AST node type"};
  }
  return p;
}

template <typename T>
const T* must(const ast::expression& e) {
  const auto p = as<T>(e);
  if (!p) {
    ADD_FAILURE() << "expected " << typeid(T).name() << " (expression)";
    throw std::runtime_error{"must<T>: wrong AST node type"};
  }
  return p;
}

}  // namespace

using err = core::error_code;

TEST(parser_test, empty_program) {
  auto h = pipeline_harness{""};
  h.lex();
  h.parse();
  ASSERT_FALSE(h.had_error());
  EXPECT_TRUE(h.ast().empty());
}

struct var_decl_case {
  std::string_view source_;
  std::string_view expected_name_;
};

struct var_decl_test : ::testing::TestWithParam<var_decl_case> {};

TEST_P(var_decl_test, parsed) {
  auto&& tc = GetParam();
  auto h = pipeline_harness{tc.source_};
  h.lex();
  h.parse();
  ASSERT_FALSE(h.had_error());
  ASSERT_EQ(h.ast().size(), 1);

  const auto decl = must<ast::var_declaration_stmt>(h.ast()[0]);
  EXPECT_EQ(decl->name_.lexeme_, tc.expected_name_);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
        various, var_decl_test,
        ::testing::Values(
            var_decl_case{"let x = 42;", "x"}, 
            var_decl_case{"let pi = 3.14;", "pi"},
            var_decl_case{"let flag = true;", "flag"},
            var_decl_case{"let s = \"hi\";", "s"}
));
// clang-format on

struct expr_case {
  std::string_view source_;
  std::string_view op_lexeme_;
  std::string_view left_name_;
};

struct binary_expr_test : ::testing::TestWithParam<expr_case> {};

TEST_P(binary_expr_test, parsed) {
  auto&& tc = GetParam();
  auto h = pipeline_harness{tc.source_};
  h.lex();
  h.parse();
  ASSERT_FALSE(h.had_error());
  ASSERT_EQ(h.ast().size(), 2);

  const auto decl = must<ast::var_declaration_stmt>(h.ast().back());
  const auto bin = must<ast::binary_expr>(decl->initializer_);
  EXPECT_EQ(bin->op_.lexeme_, tc.op_lexeme_);

  const auto left = must<ast::variable_expr>(bin->left_);
  EXPECT_EQ(left->name_.lexeme_, tc.left_name_);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
        arithmetic, binary_expr_test,
        ::testing::Values(
            expr_case{"let x = 0; let r = x + 1;", "+", "x"},
            expr_case{"let x = 0; let r = x - 1;", "-", "x"},
            expr_case{"let x = 0; let r = x * 2;", "*", "x"},
            expr_case{"let x = 0; let r = x / 2;", "/", "x"},
            expr_case{"let x = 0; let r = x % 3;", "%", "x"}
));

INSTANTIATE_TEST_SUITE_P(
        comparison, binary_expr_test,
        ::testing::Values(
            expr_case{"let x = 0; let r = x == 0;", "==", "x"},
            expr_case{"let x = 0; let r = x != 0;", "!=", "x"},
            expr_case{"let x = 0; let r = x < 0;", "<", "x"},
            expr_case{"let x = 0; let r = x <= 0;", "<=", "x"},
            expr_case{"let x = 0; let r = x > 0;", ">", "x"},
            expr_case{"let x = 0; let r = x >= 0;", ">=", "x"}
));
// clang-format on

struct unary_case {
  std::string_view source_;
  std::string_view op_lexeme_;
};

struct unary_test : ::testing::TestWithParam<unary_case> {};

TEST_P(unary_test, parsed) {
  auto&& tc = GetParam();
  auto h = pipeline_harness{tc.source_};
  h.lex();
  h.parse();
  ASSERT_FALSE(h.had_error());

  const auto decl = must<ast::var_declaration_stmt>(h.ast().back());
  const auto un = must<ast::unary_expr>(decl->initializer_);
  EXPECT_EQ(un->op_.lexeme_, tc.op_lexeme_);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
        prefix, unary_test,
        ::testing::Values(
            unary_case{"let x = 0; let r = -x;", "-"},
            unary_case{"let x = true; let r = !x;", "!"},
            unary_case{"let x = 0; let r = ++x;", "++"},
            unary_case{"let x = 0; let r = --x;", "--"}
));
// clang-format on

struct func_case {
  std::string_view source_;
  std::string_view name_;
  std::size_t param_count_;
};

struct func_decl_test : ::testing::TestWithParam<func_case> {};

TEST_P(func_decl_test, parsed) {
  auto&& tc = GetParam();
  auto h = pipeline_harness{tc.source_};
  h.lex();
  h.parse();
  ASSERT_FALSE(h.had_error());
  ASSERT_EQ(h.ast().size(), 1);

  const auto func = must<ast::func_declaration_stmt>(h.ast()[0]);
  EXPECT_EQ(func->name_.lexeme_, tc.name_);
  EXPECT_EQ(func->params_.size(), tc.param_count_);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
        various, func_decl_test,
        ::testing::Values(
            func_case{"fn main() { return 0; }", "main", 0},
            func_case{"fn foo() { }", "foo", 0}, 
            func_case{"fn add(a, b) { }", "add", 2},
            func_case{"fn f(x, y) { }", "f", 2}
));
// clang-format on

TEST(parser_test, func_body_is_flat_stmt_list) {
  auto h = pipeline_harness{"fn f() { let x = 1; let y = 2; }"};
  h.lex();
  h.parse();
  ASSERT_FALSE(h.had_error());
  ASSERT_EQ(h.ast().size(), 1);

  const auto func = must<ast::func_declaration_stmt>(h.ast()[0]);
  EXPECT_EQ(func->body_.size(), 2);
}

TEST(parser_test, func_params_capture_identifiers) {
  auto h = pipeline_harness{"fn add(a, b) { return 1; }"};
  h.lex();
  h.parse();
  ASSERT_FALSE(h.had_error());

  const auto func = must<ast::func_declaration_stmt>(h.ast()[0]);
  ASSERT_EQ(func->params_.size(), 2);
  EXPECT_EQ(func->params_[0].lexeme_, "a");
  EXPECT_EQ(func->params_[1].lexeme_, "b");
}

struct call_case {
  std::string_view source_;
  std::string_view callee_;
  std::size_t arg_count_;
};

struct call_test : ::testing::TestWithParam<call_case> {};

TEST_P(call_test, parsed) {
  auto&& tc = GetParam();
  auto h = pipeline_harness{tc.source_};
  h.lex();
  h.parse();
  ASSERT_FALSE(h.had_error());

  const auto func = must<ast::func_declaration_stmt>(h.ast()[0]);
  ASSERT_GE(func->body_.size(), 1);

  const auto es = must<ast::expression_stmt>(func->body_.back());
  const auto call = must<ast::call_expr>(es->expr_);
  EXPECT_EQ(call->callee_.lexeme_, tc.callee_);
  EXPECT_EQ(call->args_.size(), tc.arg_count_);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    various, call_test,
    ::testing::Values(
        call_case{"fn f() { print(); }", "print", 0}, 
        call_case{"fn f() { print(42); }", "print", 1},
        call_case{"fn f() { add(1, 2); }", "add", 2},
        call_case{"fn f() { let a = 0; let b = 0; let c = 0; foo(a, b, c); }", "foo", 3}
));
// clang-format on

TEST(parser_test, if_statement) {
  auto h = pipeline_harness{"fn f() { if true { return; } }"};
  h.lex();
  h.parse();
  ASSERT_FALSE(h.had_error());

  const auto func = must<ast::func_declaration_stmt>(h.ast()[0]);
  ASSERT_EQ(func->body_.size(), 1);

  const auto ifs = must<ast::if_stmt>(func->body_[0]);
  const auto cond = must<ast::literal_expr>(ifs->condition_);
  const auto value = h.literals().get(cond->pool_idx_);
  EXPECT_TRUE(value.is_bool());
  EXPECT_EQ(value.to_bool(), true);
  EXPECT_FALSE(ifs->else_block_);
}

TEST(parser_test, if_else_statement) {
  auto h = pipeline_harness{"fn f() { if true { return 0; } else { return 1; } }"};
  h.lex();
  h.parse();
  ASSERT_FALSE(h.had_error());

  const auto func = must<ast::func_declaration_stmt>(h.ast()[0]);
  ASSERT_EQ(func->body_.size(), 1);

  const auto ifs = must<ast::if_stmt>(func->body_[0]);
  EXPECT_TRUE(ifs->else_block_);
}

TEST(parser_test, while_statement) {
  auto h = pipeline_harness{"fn f() { while true { } }"};
  h.lex();
  h.parse();
  ASSERT_FALSE(h.had_error());

  const auto func = must<ast::func_declaration_stmt>(h.ast()[0]);
  ASSERT_EQ(func->body_.size(), 1);

  [[maybe_unused]] const auto ws = must<ast::while_stmt>(func->body_[0]);
}

TEST(parser_test, for_statement) {
  auto h = pipeline_harness{"fn f() { for (let i = 0; i < 10; i = i + 1) { } }"};
  h.lex();
  h.parse();
  ASSERT_FALSE(h.had_error());

  const auto func = must<ast::func_declaration_stmt>(h.ast()[0]);
  ASSERT_EQ(func->body_.size(), 1);

  const auto fs = must<ast::for_stmt>(func->body_[0]);
  EXPECT_TRUE(fs->initializer_);
  EXPECT_TRUE(fs->condition_);
  EXPECT_TRUE(fs->increment_);
}

TEST(parser_test, return_void) {
  auto h = pipeline_harness{"fn f() { return; }"};
  h.lex();
  h.parse();
  ASSERT_FALSE(h.had_error());

  const auto func = must<ast::func_declaration_stmt>(h.ast()[0]);
  const auto ret = must<ast::return_stmt>(func->body_[0]);
  EXPECT_FALSE(ret->value_);
}

TEST(parser_test, return_with_value) {
  auto h = pipeline_harness{"fn f() { return 42; }"};
  h.lex();
  h.parse();
  ASSERT_FALSE(h.had_error());

  const auto func = must<ast::func_declaration_stmt>(h.ast()[0]);
  const auto ret = must<ast::return_stmt>(func->body_[0]);
  ASSERT_TRUE(ret->value_);

  const auto lit = must<ast::literal_expr>(*ret->value_);
  const auto value = h.literals().get(lit->pool_idx_);
  EXPECT_EQ(value.to_int(), 42);
}

TEST(parser_test, struct_two_fields) {
  auto h = pipeline_harness{"struct Point { x, y }"};
  h.lex();
  h.parse();
  ASSERT_FALSE(h.had_error());
  ASSERT_EQ(h.ast().size(), 1);

  const auto decl = must<ast::struct_declaration_stmt>(h.ast()[0]);
  EXPECT_EQ(decl->name_, "Point");
  ASSERT_EQ(decl->field_names_.size(), 2);
  EXPECT_EQ(decl->field_names_[0], "x");
  EXPECT_EQ(decl->field_names_[1], "y");
  EXPECT_TRUE(decl->methods_.empty());
}

TEST(parser_test, struct_with_method) {
  auto h = pipeline_harness{"struct Counter { value, fn get() { return 0; } }"};
  h.lex();
  h.parse();
  ASSERT_FALSE(h.had_error());

  const auto decl = must<ast::struct_declaration_stmt>(h.ast()[0]);
  EXPECT_EQ(decl->name_, "Counter");
  ASSERT_EQ(decl->field_names_.size(), 1);
  ASSERT_EQ(decl->methods_.size(), 1);
  EXPECT_EQ(decl->methods_[0].name_.lexeme_, "get");
}

TEST(parser_test, member_access) {
  auto h = pipeline_harness{"fn foo(p) { p.x = 10; }"};
  h.lex();
  h.parse();
  ASSERT_FALSE(h.had_error());

  const auto func = must<ast::func_declaration_stmt>(h.ast()[0]);
  const auto es = must<ast::expression_stmt>(func->body_[0]);
  const auto assign = must<ast::assignment_expr>(es->expr_);
  const auto member = must<ast::member_access_expr>(assign->target_);
  EXPECT_EQ(member->member_.lexeme_, "x");
}

TEST(parser_test, nested_member_access) {
  auto h = pipeline_harness{"fn foo(r) { r.tl.x = 10; }"};
  h.lex();
  h.parse();
  ASSERT_FALSE(h.had_error());

  const auto func = must<ast::func_declaration_stmt>(h.ast()[0]);
  const auto es = must<ast::expression_stmt>(func->body_[0]);
  const auto assign = must<ast::assignment_expr>(es->expr_);
  const auto outer = must<ast::member_access_expr>(assign->target_);
  EXPECT_EQ(outer->member_.lexeme_, "x");

  const auto inner = must<ast::member_access_expr>(outer->object_);
  EXPECT_EQ(inner->member_.lexeme_, "tl");
}

TEST(parser_test, method_call_no_args) {
  auto h = pipeline_harness{"fn f(p) { p.inc(); }"};
  h.lex();
  h.parse();
  ASSERT_FALSE(h.had_error());

  const auto func = must<ast::func_declaration_stmt>(h.ast()[0]);
  const auto es = must<ast::expression_stmt>(func->body_[0]);
  const auto mc = must<ast::method_call_expr>(es->expr_);
  EXPECT_EQ(mc->method_.lexeme_, "inc");
  EXPECT_TRUE(mc->args_.empty());
}

TEST(parser_test, method_call_chained) {
  auto h = pipeline_harness{"fn f(p) { p.inner.method(); }"};
  h.lex();
  h.parse();
  ASSERT_FALSE(h.had_error());

  const auto func = must<ast::func_declaration_stmt>(h.ast()[0]);
  const auto es = must<ast::expression_stmt>(func->body_[0]);
  const auto mc = must<ast::method_call_expr>(es->expr_);
  EXPECT_EQ(mc->method_.lexeme_, "method");

  const auto inner = must<ast::member_access_expr>(mc->object_);
  EXPECT_EQ(inner->member_.lexeme_, "inner");
}

TEST(parser_test, method_call_on_call_result) {
  auto h = pipeline_harness{"fn f() { make_point().distance(); }"};
  h.lex();
  h.parse();
  ASSERT_FALSE(h.had_error());

  const auto func = must<ast::func_declaration_stmt>(h.ast()[0]);
  const auto es = must<ast::expression_stmt>(func->body_[0]);
  const auto mc = must<ast::method_call_expr>(es->expr_);
  EXPECT_EQ(mc->method_.lexeme_, "distance");

  EXPECT_TRUE(as<ast::call_expr>(mc->object_));
}

struct syntax_error_case {
  std::string_view source_;
  err expected_code_;
  std::string_view description_;
};

struct syntax_error_test : ::testing::TestWithParam<syntax_error_case> {};

TEST_P(syntax_error_test, reports_error) {
  auto&& tc = GetParam();
  auto h = pipeline_harness{tc.source_};
  h.lex();
  h.parse();
  ASSERT_TRUE(h.had_error());
  if (!h.had_error(tc.expected_code_)) h.render_diagnostics();
  EXPECT_TRUE(h.had_error(tc.expected_code_)) << "for: " << tc.description_;
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
        all, syntax_error_test,
        ::testing::Values(
            syntax_error_case{"let x = ;", err::expected_expression, "missing init expression"},
            syntax_error_case{"let x", err::expected_equal, "missing '='"},
            syntax_error_case{"(1 + 2", err::expected_right_paren, "unclosed paren"}
));
// clang-format on

TEST(parser_test, error_recovery) {
  auto h = pipeline_harness{"let x = ; let y = 42;"};
  h.lex();
  h.parse();
  ASSERT_TRUE(h.had_error());
  if (!h.had_error(err::expected_expression)) h.render_diagnostics();
  EXPECT_TRUE(h.had_error(err::expected_expression));
  ASSERT_EQ(h.ast().size(), 1);

  const auto decl = must<ast::var_declaration_stmt>(h.ast()[0]);
  EXPECT_EQ(decl->name_.lexeme_, "y");
}

}  // namespace cint::tests
