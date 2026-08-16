// tests/test_parser.cpp

#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "pipeline_harness.hpp"

#include <gtest/gtest.h>
#include <string_view>
#include <variant>
#include <vector>

namespace tests {

using t = core::type;

template <typename T>
const T* as(const ast::statement& stmt) {
    if (auto* p = std::get_if<ast::node<T>>(&stmt)) return p->get();
    return nullptr;
}

template <typename T>
const T* as(const ast::expression& expr) {
    if (auto* p = std::get_if<ast::node<T>>(&expr)) return p->get();
    return nullptr;
}

TEST(parser_test, empty_program) {
    pipeline_harness h{""};
    ASSERT_TRUE(h.lex());
    ASSERT_TRUE(h.parse());
    EXPECT_TRUE(h.ast().empty());
    EXPECT_FALSE(h.had_error());
}

struct var_decl_case {
    std::string_view source_;
    std::string_view expected_name_;
    t expected_type_;
    bool has_initializer_;
};

class var_decl_test : public ::testing::TestWithParam<var_decl_case> {};

TEST_P(var_decl_test, parsed) {
    auto&& tc = GetParam();
    pipeline_harness h{tc.source_};
    ASSERT_TRUE(h.lex());
    ASSERT_TRUE(h.parse());
    ASSERT_EQ(h.ast().size(), 1);
    auto&& decl = as<ast::var_declaration_stmt>(h.ast()[0]);
    ASSERT_TRUE(decl);
    EXPECT_EQ(decl->name_.lexeme_, tc.expected_name_);
    EXPECT_EQ(decl->type_, tc.expected_type_);
    EXPECT_EQ(decl->initializer_.has_value(), tc.has_initializer_);
    EXPECT_FALSE(h.had_error());
}
// clang-format off
INSTANTIATE_TEST_SUITE_P(
        without_init, var_decl_test,
        ::testing::Values(
            var_decl_case{"int x;", "x", t::int_type(), false},
            var_decl_case{"double pi;", "pi", t::double_type(), false},
            var_decl_case{"bool flag;", "flag", t::bool_type(), false},
            var_decl_case{"string s;", "s", t::string_type(), false}));

INSTANTIATE_TEST_SUITE_P(
        with_init, var_decl_test,
        ::testing::Values(
            var_decl_case{"int x = 42;", "x", t::int_type(), true},
            var_decl_case{"double pi = 3.14;", "pi", t::double_type(), true},
            var_decl_case{"bool f = true;", "f", t::bool_type(), true}
));

struct expr_case {
    std::string_view source_;
    std::string_view op_lexeme_;
    std::string_view left_name_;
    std::string_view right_lexeme_;
};

class binary_expr_test : public ::testing::TestWithParam<expr_case> {};

TEST_P(binary_expr_test, parsed) {
    auto&& tc = GetParam();
    pipeline_harness h{tc.source_};
    ASSERT_TRUE(h.lex());
    ASSERT_TRUE(h.parse());
    ASSERT_EQ(h.ast().size(), 1);
    auto&& decl = as<ast::var_declaration_stmt>(h.ast()[0]);
    ASSERT_TRUE(decl);
    ASSERT_TRUE(decl->initializer_);
    auto&& bin = as<ast::binary_expr>(*decl->initializer_);
    ASSERT_TRUE(bin);
    EXPECT_EQ(bin->op_.lexeme_, tc.op_lexeme_);
    auto&& left = as<ast::variable_expr>(bin->left_);
    ASSERT_TRUE(left);
    EXPECT_EQ(left->name_.lexeme_, tc.left_name_);
    auto&& right = as<ast::literal_expr>(bin->right_);
    ASSERT_TRUE(right);
    EXPECT_TRUE(right->value_.is_int());
    EXPECT_EQ(right->value_.to_int(), std::stoll(std::string(tc.right_lexeme_)));
}

INSTANTIATE_TEST_SUITE_P(
        arithmetic, binary_expr_test,
        ::testing::Values(
            expr_case{"int r = x + 1;", "+", "x", "1"},
            expr_case{"int r = x - 1;", "-", "x", "1"},
            expr_case{"int r = x * 2;", "*", "x", "2"},
            expr_case{"int r = x / 2;", "/", "x", "2"},
            expr_case{"int r = x % 3;", "%", "x", "3"}
));

INSTANTIATE_TEST_SUITE_P(
        comparison, binary_expr_test,
        ::testing::Values(
            expr_case{"bool r = x == 0;", "==", "x", "0"}, 
            expr_case{"bool r = x != 0;", "!=", "x", "0"},
            expr_case{"bool r = x < 0;", "<", "x", "0"}, 
            expr_case{"bool r = x <= 0;", "<=", "x", "0"},
            expr_case{"bool r = x > 0;", ">", "x", "0"}, 
            expr_case{"bool r = x >= 0;", ">=", "x", "0"}
));

TEST(parser_test, assignment_expression) {
    pipeline_harness h{"int x; x = 42;"};
    ASSERT_TRUE(h.lex());
    ASSERT_TRUE(h.parse());
    ASSERT_GE(h.ast().size(), 2);
    auto&& es = as<ast::expression_stmt>(h.ast()[1]);
    ASSERT_TRUE(es);
    auto&& assign = as<ast::assignment_expr>(es->expr_);
    ASSERT_TRUE(assign);
    EXPECT_EQ(assign->op_.lexeme_, "=");
    auto&& var = as<ast::variable_expr>(assign->target_);
    ASSERT_TRUE(var);
    EXPECT_EQ(var->name_.lexeme_, "x");
    auto&& lit = as<ast::literal_expr>(assign->value_);
    ASSERT_TRUE(lit);
    EXPECT_TRUE(lit->value_.is_int());
    EXPECT_EQ(lit->value_.to_int(), 42);
}

TEST(parser_test, multiplication_before_addition) {
    pipeline_harness h{"int r = 1 + 2 * 3;"};
    ASSERT_TRUE(h.lex());
    ASSERT_TRUE(h.parse());
    ASSERT_EQ(h.ast().size(), 1);
    auto&& decl = as<ast::var_declaration_stmt>(h.ast()[0]);
    ASSERT_TRUE(decl);
    auto&& bin = as<ast::binary_expr>(*decl->initializer_);
    ASSERT_TRUE(bin);
    EXPECT_EQ(bin->op_.lexeme_, "+");
    auto&& left = as<ast::literal_expr>(bin->left_);
    ASSERT_TRUE(left);
    EXPECT_TRUE(left->value_.is_int());
    EXPECT_EQ(left->value_.to_int(), 1);
    auto&& right = as<ast::binary_expr>(bin->right_);
    ASSERT_TRUE(right);
    EXPECT_EQ(right->op_.lexeme_, "*");
}

TEST(parser_test, grouping_overrides_precedence) {
    pipeline_harness h{"int r = (1 + 2) * 3;"};
    ASSERT_TRUE(h.lex());
    ASSERT_TRUE(h.parse());
    auto&& decl = as<ast::var_declaration_stmt>(h.ast()[0]);
    ASSERT_TRUE(decl);
    auto&& bin = as<ast::binary_expr>(*decl->initializer_);
    ASSERT_TRUE(bin);
    EXPECT_EQ(bin->op_.lexeme_, "*");
    auto&& left = as<ast::binary_expr>(bin->left_);
    ASSERT_TRUE(left);
    EXPECT_EQ(left->op_.lexeme_, "+");
}

struct unary_case {
    std::string_view source_;
    std::string_view op_lexeme_;
    std::string_view operand_name_;
};

class unary_test : public ::testing::TestWithParam<unary_case> {};

TEST_P(unary_test, parsed) {
    auto&& tc = GetParam();
    pipeline_harness h{tc.source_};
    ASSERT_TRUE(h.lex());
    ASSERT_TRUE(h.parse());
    auto&& decl = as<ast::var_declaration_stmt>(h.ast()[0]);
    ASSERT_TRUE(decl);
    auto&& un = as<ast::unary_expr>(*decl->initializer_);
    ASSERT_TRUE(un);
    EXPECT_EQ(un->op_.lexeme_, tc.op_lexeme_);
    auto&& var = as<ast::variable_expr>(un->operand_);
    ASSERT_TRUE(var);
    EXPECT_EQ(var->name_.lexeme_, tc.operand_name_);
}

INSTANTIATE_TEST_SUITE_P(
        prefix, unary_test,
        ::testing::Values(
            unary_case{"int r = -x;", "-", "x"}, 
            unary_case{"int r = !x;", "!", "x"},
            unary_case{"int r = ++x;", "++", "x"},
            unary_case{"int r = --x;", "--", "x"}
));

struct func_case {
    std::string_view source_;
    std::string_view name_;
    t return_type_;
    size_t param_count_;
};

class func_decl_test : public ::testing::TestWithParam<func_case> {};

TEST_P(func_decl_test, parsed) {
    auto&& tc = GetParam();
    pipeline_harness h{tc.source_};
    ASSERT_TRUE(h.lex());
    ASSERT_TRUE(h.parse());
    ASSERT_EQ(h.ast().size(), 1);
    auto&& func = as<ast::func_declaration_stmt>(h.ast()[0]);
    ASSERT_TRUE(func);
    EXPECT_EQ(func->name_.lexeme_, tc.name_);
    EXPECT_EQ(func->return_type_, tc.return_type_);
    EXPECT_EQ(func->params_.size(), tc.param_count_);
}

INSTANTIATE_TEST_SUITE_P(
        various, func_decl_test,
        ::testing::Values(
            func_case{"int main() { return 0; }", "main", t::int_type(), 0},
            func_case{"void foo() { }", "foo", t::void_type(), 0},
            func_case{"int add(int a, int b) { }", "add", t::int_type(), 2},
            func_case{"double f(int x, double y) { }", "f", t::double_type(), 2}
));

struct call_case {
    std::string_view source_;
    std::string_view callee_;
    size_t arg_count_;
};

class call_test : public ::testing::TestWithParam<call_case> {};

TEST_P(call_test, parsed) {
    auto&& tc = GetParam();
    pipeline_harness h{tc.source_};
    ASSERT_TRUE(h.lex());
    ASSERT_TRUE(h.parse());
    auto&& func = as<ast::func_declaration_stmt>(h.ast()[0]);
    ASSERT_TRUE(func);
    auto&& block = as<ast::block_stmt>(func->block_);
    ASSERT_TRUE(block);
    ASSERT_GE(block->statements_.size(), 1);
    auto&& es = as<ast::expression_stmt>(block->statements_[0]);
    ASSERT_TRUE(es);
    auto&& call = as<ast::call_expr>(es->expr_);
    ASSERT_TRUE(call);
    EXPECT_EQ(call->callee_.lexeme_, tc.callee_);
    EXPECT_EQ(call->args_.size(), tc.arg_count_);
}

INSTANTIATE_TEST_SUITE_P(
        various, call_test,
        ::testing::Values(
            call_case{"void f() { print(); }", "print", 0},
            call_case{"void f() { print_int(42); }", "print_int", 1},
            call_case{"void f() { add(1, 2); }", "add", 2},
            call_case{"void f() { foo(a, b, c); }", "foo", 3}
));

TEST(parser_test, if_statement) {
    pipeline_harness h{"void f() { if (true) { return; } }"};
    ASSERT_TRUE(h.lex());
    ASSERT_TRUE(h.parse());
    ASSERT_EQ(h.ast().size(), 1);
    auto&& func = as<ast::func_declaration_stmt>(h.ast()[0]);
    ASSERT_TRUE(func);
    auto&& block = as<ast::block_stmt>(func->block_);
    ASSERT_TRUE(block);
    ASSERT_EQ(block->statements_.size(), 1);
    auto&& ifs = as<ast::if_stmt>(block->statements_[0]);
    ASSERT_TRUE(ifs);
    auto&& cond = as<ast::literal_expr>(ifs->condition_);
    ASSERT_TRUE(cond);
    EXPECT_TRUE(cond->value_.is_bool());
    EXPECT_EQ(cond->value_.to_bool(), true);
    EXPECT_FALSE(ifs->else_block_);
}

TEST(parser_test, if_else_statement) {
    pipeline_harness h{"void f() { if (x) { return 0; } else { return 1; } }"};
    ASSERT_TRUE(h.lex());
    ASSERT_TRUE(h.parse());
    auto&& func = as<ast::func_declaration_stmt>(h.ast()[0]);
    ASSERT_TRUE(func);
    auto&& block = as<ast::block_stmt>(func->block_);
    ASSERT_TRUE(block);
    auto&& ifs = as<ast::if_stmt>(block->statements_[0]);
    ASSERT_TRUE(ifs);
    EXPECT_TRUE(ifs->else_block_);
}

TEST(parser_test, while_statement) {
    pipeline_harness h{"void f() { while (x < 10) { x = x + 1; } }"};
    ASSERT_TRUE(h.lex());
    ASSERT_TRUE(h.parse());
    auto&& func = as<ast::func_declaration_stmt>(h.ast()[0]);
    ASSERT_TRUE(func);
    auto&& block = as<ast::block_stmt>(func->block_);
    ASSERT_TRUE(block);
    auto&& ws = as<ast::while_stmt>(block->statements_[0]);
    ASSERT_TRUE(ws);
}

TEST(parser_test, for_statement) {
    pipeline_harness h{"void f() { for (int i = 0; i < 10; i = i + 1) { } }"};
    ASSERT_TRUE(h.lex());
    ASSERT_TRUE(h.parse());
    auto&& func = as<ast::func_declaration_stmt>(h.ast()[0]);
    ASSERT_TRUE(func);
    auto&& block = as<ast::block_stmt>(func->block_);
    ASSERT_TRUE(block);
    auto&& fs = as<ast::for_stmt>(block->statements_[0]);
    ASSERT_TRUE(fs);
    EXPECT_TRUE(fs->initializer_);
    EXPECT_TRUE(fs->condition_);
    EXPECT_TRUE(fs->increment_);
}

TEST(parser_test, return_void) {
    pipeline_harness h{"void f() { return; }"};
    ASSERT_TRUE(h.lex());
    ASSERT_TRUE(h.parse());
    auto&& func = as<ast::func_declaration_stmt>(h.ast()[0]);
    ASSERT_TRUE(func);
    auto&& block = as<ast::block_stmt>(func->block_);
    ASSERT_TRUE(block);
    auto&& ret = as<ast::return_stmt>(block->statements_[0]);
    ASSERT_TRUE(ret);
    EXPECT_FALSE(ret->value_);
}

TEST(parser_test, return_with_value) {
    pipeline_harness h{"int f() { return 42; }"};
    ASSERT_TRUE(h.lex());
    ASSERT_TRUE(h.parse());
    auto&& func = as<ast::func_declaration_stmt>(h.ast()[0]);
    ASSERT_TRUE(func);
    auto&& block = as<ast::block_stmt>(func->block_);
    ASSERT_TRUE(block);
    auto&& ret = as<ast::return_stmt>(block->statements_[0]);
    ASSERT_TRUE(ret);
    EXPECT_TRUE(ret->value_);
    auto&& lit = as<ast::literal_expr>(*ret->value_);
    ASSERT_TRUE(lit);
    EXPECT_TRUE(lit->value_.is_int());
    EXPECT_EQ(lit->value_.to_int(), 42);
}

struct syntax_error_case {
    std::string_view source_;
    std::string_view description_;
};

class syntax_error_test : public ::testing::TestWithParam<syntax_error_case> {};

TEST_P(syntax_error_test, reports_error) {
    auto&& tc = GetParam();
    pipeline_harness h{tc.source_};
    h.lex();
    h.parse();
    EXPECT_TRUE(h.had_error()) << "Expected error for: " << tc.description_;
}

INSTANTIATE_TEST_SUITE_P(
        all, syntax_error_test,
        ::testing::Values(
            syntax_error_case{"int x = ;", "missing init expression"},
            syntax_error_case{"int x", "missing semicolon"},
            syntax_error_case{"(1 + 2", "unclosed paren"},
            syntax_error_case{"int f( { }", "missing right paren in func"}
));
// clang-format on

TEST(parser_test, error_recovery) {
    pipeline_harness h{"int x = ; int y = 42;"};
    ASSERT_TRUE(h.lex());
    h.parse();
    EXPECT_TRUE(h.had_error());
    ASSERT_EQ(h.ast().size(), 1);
    auto&& decl = as<ast::var_declaration_stmt>(h.ast()[0]);
    ASSERT_TRUE(decl);
    EXPECT_EQ(decl->name_.lexeme_, "y");
}

TEST(parser_test, array_declaration) {
    pipeline_harness h{"int arr[10];"};
    ASSERT_TRUE(h.lex());
    ASSERT_TRUE(h.parse());
    auto&& decl = as<ast::var_declaration_stmt>(h.ast()[0]);
    ASSERT_TRUE(decl);
    EXPECT_TRUE(decl->type_.is_array());
    EXPECT_EQ(decl->type_.array_size(), 10);
    EXPECT_TRUE(decl->type_.element_type().is_int());
}

TEST(parser_test, array_initializer) {
    pipeline_harness h{"int arr[] = {1, 2, 3};"};
    ASSERT_TRUE(h.lex());
    ASSERT_TRUE(h.parse());
    auto&& decl = as<ast::var_declaration_stmt>(h.ast()[0]);
    ASSERT_TRUE(decl);
    ASSERT_TRUE(decl->initializer_);
    auto&& list = as<ast::initializer_list_expr>(*decl->initializer_);
    ASSERT_TRUE(list);
    EXPECT_EQ(list->elements_.size(), 3);
}

TEST(parser_test, struct_declaration_empty) {
    pipeline_harness h{"struct Point { int x; int y; };"};
    ASSERT_TRUE(h.lex());
    ASSERT_TRUE(h.parse());
    ASSERT_EQ(h.ast().size(), 1);
    auto&& decl = as<ast::struct_declaration_stmt>(h.ast()[0]);
    ASSERT_TRUE(decl);
    EXPECT_EQ(decl->name_.lexeme_, "Point");
    EXPECT_EQ(decl->type_.struct_fields().size(), 2);
    EXPECT_EQ(decl->type_.struct_fields()[0].first, "x");
    EXPECT_EQ(decl->type_.struct_fields()[1].first, "y");
    EXPECT_FALSE(h.had_error());
}

TEST(parser_test, struct_declaration_nested) {
    pipeline_harness h{
        "struct Point { int x; int y; };"
        "struct Rect { struct Point tl; struct Point br; };"};
    ASSERT_TRUE(h.lex());
    ASSERT_TRUE(h.parse());
    ASSERT_EQ(h.ast().size(), 2);
    auto&& rect = as<ast::struct_declaration_stmt>(h.ast()[1]);
    ASSERT_TRUE(rect);
    EXPECT_EQ(rect->name_.lexeme_, "Rect");
    EXPECT_EQ(rect->type_.struct_fields().size(), 2);
    EXPECT_FALSE(h.had_error());
}

TEST(parser_test, struct_variable_declaration) {
    pipeline_harness h{"struct Point { int x; int y; }; struct Point p;"};
    ASSERT_TRUE(h.lex());
    ASSERT_TRUE(h.parse());
    ASSERT_EQ(h.ast().size(), 2);
    auto&& decl = as<ast::var_declaration_stmt>(h.ast()[1]);
    ASSERT_TRUE(decl);
    EXPECT_EQ(decl->name_.lexeme_, "p");
    EXPECT_TRUE(decl->type_.is_struct());
    EXPECT_EQ(decl->type_.struct_name(), "Point");
    EXPECT_FALSE(h.had_error());
}

TEST(parser_test, member_access) {
    pipeline_harness h{
        "void foo(struct Point p) {"
        "  p.x = 10;"
        "}"};
    ASSERT_TRUE(h.lex());
    ASSERT_TRUE(h.parse());
    ASSERT_EQ(h.ast().size(), 1);
    auto&& func = as<ast::func_declaration_stmt>(h.ast()[0]);
    ASSERT_TRUE(func);
    auto&& block = as<ast::block_stmt>(func->block_);
    ASSERT_TRUE(block);
    ASSERT_EQ(block->statements_.size(), 1);
    auto&& es = as<ast::expression_stmt>(block->statements_[0]);
    ASSERT_TRUE(es);
    auto&& assign = as<ast::assignment_expr>(es->expr_);
    ASSERT_TRUE(assign);
    auto&& member = as<ast::member_access_expr>(assign->target_);
    ASSERT_TRUE(member);
    EXPECT_EQ(member->member_.lexeme_, "x");
    EXPECT_FALSE(h.had_error());
}

TEST(parser_test, nested_member_access) {
    pipeline_harness h{
        "void foo(struct Rect r) {"
        "  r.tl.x = 10;"
        "}"};
    ASSERT_TRUE(h.lex());
    ASSERT_TRUE(h.parse());
    auto&& func = as<ast::func_declaration_stmt>(h.ast()[0]);
    ASSERT_TRUE(func);
    auto&& block = as<ast::block_stmt>(func->block_);
    ASSERT_TRUE(block);
    auto&& es = as<ast::expression_stmt>(block->statements_[0]);
    ASSERT_TRUE(es);
    auto&& assign = as<ast::assignment_expr>(es->expr_);
    ASSERT_TRUE(assign);
    auto&& outer = as<ast::member_access_expr>(assign->target_);
    ASSERT_TRUE(outer);
    EXPECT_EQ(outer->member_.lexeme_, "x");
    auto&& inner = as<ast::member_access_expr>(outer->object_);
    ASSERT_TRUE(inner);
    EXPECT_EQ(inner->member_.lexeme_, "tl");
    EXPECT_FALSE(h.had_error());
}

TEST(parser_test, struct_with_string_field) {
    pipeline_harness h{"struct Person { string name; int age; };"};
    ASSERT_TRUE(h.lex());
    ASSERT_TRUE(h.parse());
    ASSERT_EQ(h.ast().size(), 1);
    auto&& decl = as<ast::struct_declaration_stmt>(h.ast()[0]);
    ASSERT_TRUE(decl);
    EXPECT_EQ(decl->type_.struct_fields().size(), 2);
    EXPECT_TRUE(decl->type_.struct_fields()[0].second.is_string());
    EXPECT_FALSE(h.had_error());
}

}  // namespace tests
