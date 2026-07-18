// test_parser.cpp

#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/error/error_report.hpp"
#include "core/utils/arena.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

#include <gtest/gtest.h>
#include <string>
#include <variant>
#include <vector>

namespace tests {

using t = core::type;

class parser_harness {
public:
    parser_harness(std::string source) : source_code_(std::move(source)), reporter_(source_code_), mr_(arena_) {
        lexer lex(source_code_, reporter_, mr_);
        tokens_ = lex.scan_tokens();
        parser p(tokens_, reporter_, arena_, mr_);
        ast_ = p.parse();
    }

    const parser::ast_list& ast() const noexcept { return ast_; }
    bool had_error() const noexcept { return reporter_.has_error(); }

private:
    std::string source_code_;
    core::error_reporter reporter_;
    core::arena arena_;
    core::arena_memory_resource mr_;
    lexer::token_list tokens_;
    parser::ast_list ast_;
};

template <typename T>
const T* as(const ast::statement& stmt) {
    return std::get_if<T>(&stmt.data_);
}

template <typename T>
const T* as(const ast::expression& expr) {
    if constexpr (std::is_same_v<T, ast::literal_expr>) {
        return std::get_if<ast::literal_expr>(&expr);
    } else if constexpr (std::is_same_v<T, ast::variable_expr>) {
        return std::get_if<ast::variable_expr>(&expr);
    } else {
        if (auto* p = std::get_if<core::arena_ptr<T>>(&expr)) { return p->get(); }
        return static_cast<const T*>(nullptr);
    }
}

bool is_literal(const ast::expression& expr, std::string_view value) {
    auto* lit = as<ast::literal_expr>(expr);
    return lit && lit->value_.lexeme_ == value;
}

bool is_variable(const ast::expression& expr, std::string_view name) {
    auto* var = as<ast::variable_expr>(expr);
    return var && var->name_.lexeme_ == name;
}

TEST(parser_test, empty_program) {
    parser_harness h("");
    EXPECT_TRUE(h.ast().empty());
    EXPECT_FALSE(h.had_error());
}

struct var_decl_case {
    std::string_view source_;
    std::string_view expected_name_;
    t expected_type_;
    bool has_initializer_;
    std::string_view init_lexeme_{};
};

class var_decl_test : public ::testing::TestWithParam<var_decl_case> {};

TEST_P(var_decl_test, parsed) {
    const auto& tc = GetParam();
    parser_harness h(std::string{tc.source_});

    ASSERT_EQ(h.ast().size(), 1);
    auto* decl = as<ast::var_declaration>(*h.ast()[0]);
    ASSERT_NE(decl, nullptr);
    EXPECT_EQ(decl->name_.lexeme_, tc.expected_name_);
    EXPECT_EQ(decl->type_, tc.expected_type_);
    EXPECT_EQ(decl->initializer_.has_value(), tc.has_initializer_);
    if (tc.has_initializer_) {
        ASSERT_TRUE(decl->initializer_.has_value());
        EXPECT_TRUE(is_literal(*decl->initializer_, tc.init_lexeme_));
    }
    EXPECT_FALSE(h.had_error());
}

INSTANTIATE_TEST_SUITE_P(without_init, var_decl_test,
                         ::testing::Values(var_decl_case{"int x;", "x", t::int_type(), false},
                                           var_decl_case{"double pi;", "pi", t::double_type(), false},
                                           var_decl_case{"bool flag;", "flag", t::bool_type(), false},
                                           var_decl_case{"string s;", "s", t::string_type(), false}));

INSTANTIATE_TEST_SUITE_P(with_init, var_decl_test,
                         ::testing::Values(var_decl_case{"int x = 42;", "x", t::int_type(), true, "42"},
                                           var_decl_case{"double pi = 3.14;", "pi", t::double_type(), true, "3.14"},
                                           var_decl_case{"bool f = true;", "f", t::bool_type(), true, "true"}));

struct expr_case {
    std::string_view source_;
    std::string_view op_lexeme_;
    std::string_view left_lexeme_;
    std::string_view right_lexeme_;
};

class binary_expr_test : public ::testing::TestWithParam<expr_case> {};

TEST_P(binary_expr_test, parsed) {
    const auto& tc = GetParam();
    parser_harness h(std::string{tc.source_});

    ASSERT_EQ(h.ast().size(), 1);
    auto* decl = as<ast::var_declaration>(*h.ast()[0]);
    ASSERT_NE(decl, nullptr);
    ASSERT_TRUE(decl->initializer_.has_value());

    auto* bin = as<ast::binary_expr>(*decl->initializer_);
    ASSERT_NE(bin, nullptr) << "Expected binary expression for source: " << tc.source_;
    EXPECT_EQ(bin->op_.lexeme_, tc.op_lexeme_);
    EXPECT_TRUE(is_variable(bin->left_, tc.left_lexeme_));
    EXPECT_TRUE(is_literal(bin->right_, tc.right_lexeme_));
    EXPECT_FALSE(h.had_error());
}

TEST(parser_test, assignment_expression) {
    parser_harness h("int x; x = 42;");

    ASSERT_GE(h.ast().size(), 2);
    auto* es = as<ast::expression_stmt>(*h.ast()[1]);
    ASSERT_NE(es, nullptr);

    auto* assign = as<ast::assignment_expr>(es->expr_);
    ASSERT_NE(assign, nullptr);
    EXPECT_EQ(assign->op_.lexeme_, "=");
    EXPECT_TRUE(is_variable(assign->target_, "x"));
    EXPECT_TRUE(is_literal(assign->value_, "42"));
}

INSTANTIATE_TEST_SUITE_P(arithmetic, binary_expr_test,
                         ::testing::Values(expr_case{"int r = x + 1;", "+", "x", "1"},
                                           expr_case{"int r = x - 1;", "-", "x", "1"},
                                           expr_case{"int r = x * 2;", "*", "x", "2"},
                                           expr_case{"int r = x / 2;", "/", "x", "2"},
                                           expr_case{"int r = x % 3;", "%", "x", "3"}));

INSTANTIATE_TEST_SUITE_P(
    comparison, binary_expr_test,
    ::testing::Values(expr_case{"bool r = x == 0;", "==", "x", "0"}, expr_case{"bool r = x != 0;", "!=", "x", "0"},
                      expr_case{"bool r = x < 0;", "<", "x", "0"}, expr_case{"bool r = x <= 0;", "<=", "x", "0"},
                      expr_case{"bool r = x > 0;", ">", "x", "0"}, expr_case{"bool r = x >= 0;", ">=", "x", "0"}));

TEST(parser_test, multiplication_before_addition) {
    parser_harness h("int r = 1 + 2 * 3;");

    ASSERT_EQ(h.ast().size(), 1);
    auto* decl = as<ast::var_declaration>(*h.ast()[0]);
    ASSERT_NE(decl, nullptr);

    auto* bin = as<ast::binary_expr>(*decl->initializer_);
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op_.lexeme_, "+");
    EXPECT_TRUE(is_literal(bin->left_, "1"));

    auto* right = as<ast::binary_expr>(bin->right_);
    ASSERT_NE(right, nullptr);
    EXPECT_EQ(right->op_.lexeme_, "*");
}

TEST(parser_test, grouping_overrides_precedence) {
    parser_harness h("int r = (1 + 2) * 3;");

    auto* decl = as<ast::var_declaration>(*h.ast()[0]);
    auto* bin = as<ast::binary_expr>(*decl->initializer_);
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op_.lexeme_, "*");

    auto* left = as<ast::binary_expr>(bin->left_);
    ASSERT_NE(left, nullptr);
    EXPECT_EQ(left->op_.lexeme_, "+");
}

struct unary_case {
    std::string_view source_;
    std::string_view op_lexeme_;
    std::string_view operand_name_;
};

class unary_test : public ::testing::TestWithParam<unary_case> {};

TEST_P(unary_test, parsed) {
    const auto& tc = GetParam();
    parser_harness h(std::string{tc.source_});

    auto* decl = as<ast::var_declaration>(*h.ast()[0]);
    ASSERT_NE(decl, nullptr);

    auto* un = as<ast::unary_expr>(*decl->initializer_);
    ASSERT_NE(un, nullptr);
    EXPECT_EQ(un->op_.lexeme_, tc.op_lexeme_);
    EXPECT_TRUE(is_variable(un->operand_, tc.operand_name_));
}

INSTANTIATE_TEST_SUITE_P(prefix, unary_test,
                         ::testing::Values(unary_case{"int r = -x;", "-", "x"}, unary_case{"int r = !x;", "!", "x"},
                                           unary_case{"int r = ++x;", "++", "x"},
                                           unary_case{"int r = --x;", "--", "x"}));

struct func_case {
    std::string_view source_;
    std::string_view name_;
    t return_type_;
    size_t param_count_;
};

class func_decl_test : public ::testing::TestWithParam<func_case> {};

TEST_P(func_decl_test, parsed) {
    const auto& tc = GetParam();
    parser_harness h(std::string{tc.source_});

    ASSERT_EQ(h.ast().size(), 1);
    auto* func = as<ast::func_declaration>(*h.ast()[0]);
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->name_.lexeme_, tc.name_);
    EXPECT_EQ(func->return_type_, tc.return_type_);
    EXPECT_EQ(func->params_.size(), tc.param_count_);
}

INSTANTIATE_TEST_SUITE_P(various, func_decl_test,
                         ::testing::Values(func_case{"int main() { return 0; }", "main", t::int_type(), 0},
                                           func_case{"void foo() { }", "foo", t::void_type(), 0},
                                           func_case{"int add(int a, int b) { }", "add", t::int_type(), 2},
                                           func_case{"double f(int x, double y) { }", "f", t::double_type(), 2}));

struct call_case {
    std::string_view source_;
    std::string_view callee_;
    size_t arg_count_;
};

class call_test : public ::testing::TestWithParam<call_case> {};

TEST_P(call_test, parsed) {
    const auto& tc = GetParam();
    parser_harness h(std::string{tc.source_});

    auto* func = as<ast::func_declaration>(*h.ast()[0]);
    ASSERT_NE(func, nullptr);
    ASSERT_GE(func->block_->statements_.size(), 1);

    auto* es = as<ast::expression_stmt>(*func->block_->statements_[0]);
    ASSERT_NE(es, nullptr);

    auto* call = as<ast::call_expr>(es->expr_);
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->callee_.lexeme_, tc.callee_);
    EXPECT_EQ(call->args_.size(), tc.arg_count_);
}

INSTANTIATE_TEST_SUITE_P(various, call_test,
                         ::testing::Values(call_case{"void f() { print(); }", "print", 0},
                                           call_case{"void f() { print_int(42); }", "print_int", 1},
                                           call_case{"void f() { add(1, 2); }", "add", 2},
                                           call_case{"void f() { foo(a, b, c); }", "foo", 3}));

TEST(parser_test, if_statement) {
    parser_harness h("void f() { if (true) { return; } }");
    ASSERT_EQ(h.ast().size(), 1);
    auto* func = as<ast::func_declaration>(*h.ast()[0]);
    ASSERT_NE(func, nullptr);
    ASSERT_EQ(func->block_->statements_.size(), 1);
    auto* ifs = as<ast::if_stmt>(*func->block_->statements_[0]);
    ASSERT_NE(ifs, nullptr);
    EXPECT_TRUE(is_literal(ifs->condition_, "true"));
    EXPECT_NE(ifs->then_block_, nullptr);
    EXPECT_EQ(ifs->else_block_, nullptr);
}

TEST(parser_test, if_else_statement) {
    parser_harness h("void f() { if (x) { return 0; } else { return 1; } }");
    auto* func = as<ast::func_declaration>(*h.ast()[0]);
    auto* ifs = as<ast::if_stmt>(*func->block_->statements_[0]);
    ASSERT_NE(ifs, nullptr);
    EXPECT_NE(ifs->else_block_, nullptr);
}

TEST(parser_test, while_statement) {
    parser_harness h("void f() { while (x < 10) { x = x + 1; } }");
    auto* func = as<ast::func_declaration>(*h.ast()[0]);
    auto* ws = as<ast::while_stmt>(*func->block_->statements_[0]);
    ASSERT_NE(ws, nullptr);
    EXPECT_NE(ws->block_, nullptr);
}

TEST(parser_test, for_statement) {
    parser_harness h(
        "void f() {"
        "  for (int i = 0; i < 10; i = i + 1) { }"
        "}");
    auto* func = as<ast::func_declaration>(*h.ast()[0]);
    auto* fs = as<ast::for_stmt>(*func->block_->statements_[0]);
    ASSERT_NE(fs, nullptr);
    EXPECT_NE(fs->initializer_, nullptr);
    EXPECT_TRUE(fs->condition_.has_value());
    EXPECT_TRUE(fs->increment_.has_value());
    EXPECT_NE(fs->block_, nullptr);
}

TEST(parser_test, return_void) {
    parser_harness h("void f() { return; }");
    auto* func = as<ast::func_declaration>(*h.ast()[0]);
    auto* ret = as<ast::return_stmt>(*func->block_->statements_[0]);
    ASSERT_NE(ret, nullptr);
    EXPECT_FALSE(ret->value_.has_value());
}

TEST(parser_test, return_with_value) {
    parser_harness h("int f() { return 42; }");
    auto* func = as<ast::func_declaration>(*h.ast()[0]);
    auto* ret = as<ast::return_stmt>(*func->block_->statements_[0]);
    ASSERT_NE(ret, nullptr);
    EXPECT_TRUE(ret->value_.has_value());
    EXPECT_TRUE(is_literal(*ret->value_, "42"));
}

struct syntax_error_case {
    std::string_view source_;
    std::string_view description_;
};

class syntax_error_test : public ::testing::TestWithParam<syntax_error_case> {};

TEST_P(syntax_error_test, reports_error) {
    const auto& tc = GetParam();
    parser_harness h(std::string{tc.source_});
    EXPECT_TRUE(h.had_error()) << "Expected error for: " << tc.description_;
}

INSTANTIATE_TEST_SUITE_P(all, syntax_error_test,
                         ::testing::Values(syntax_error_case{"int x = ;", "missing init expression"},
                                           syntax_error_case{"int x", "missing semicolon"},
                                           syntax_error_case{"(1 + 2", "unclosed paren"},
                                           syntax_error_case{"int f( { }", "missing right paren in func"}));

}  // namespace tests
