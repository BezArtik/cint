#include <gtest/gtest.h>
#include "parser/parser.hpp"
#include "lexer/lexer.hpp"
#include "semantics/type_check.hpp"
#include "core/error/error_report.hpp"
#include <string>
#include <vector>

namespace tests {

class semantics_harness {
public:
    semantics_harness(std::string source)
        : source_code_(std::move(source))
        , reporter_(source_code_) {
        lexer::lexer lex(source_code_, reporter_);
        tokens_ = lex.scan_tokens();
        parser::parser p(tokens_, reporter_);
        ast_ = p.parse();
        semantics::type_checker checker(reporter_);
        check_ok_ = checker.check(ast_);
        had_error_ = reporter_.has_error();
    }

    bool ok() const { return check_ok_ && !had_error_; }
    bool had_error() const { return had_error_; }

private:
    std::string source_code_;
    core::error_reporter reporter_;
    std::vector<core::token> tokens_;
    std::vector<ast::stmt_ptr> ast_;
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
    semantics_harness h(std::string{ tc.source_ });
    EXPECT_TRUE(h.ok()) << "Unexpected error for: " << tc.description_;
}

INSTANTIATE_TEST_SUITE_P(basics, valid_program_test, ::testing::Values(
    valid_program_case{ "int main() { return 0; }",                                                 "minimal" },
    valid_program_case{ "void f() { }",                                                             "empty void func" },
    valid_program_case{ "int f(int a) { return a; }",                                               "param return" },
    valid_program_case{ "int f() { int x = 42; return x; }",                                        "var init" },
    valid_program_case{ "double f() { int x = 1; return x; }",                                      "int to double" },
    valid_program_case{ "int f() { int x; x = 5; return x; }",                                      "assign" },
    valid_program_case{ "int f() { int x = 1; x += 2; return x; }",                                 "compound assign" },
    valid_program_case{ "int f() { if (true) { return 1; } return 0; }",                            "if true" },
    valid_program_case{ "int f() { if (true) { return 1; } else { return 2; } }",                   "if else" },
    valid_program_case{ "int f() { while (true) { } return 0; }",                                   "while" },
    valid_program_case{ "int f() { for (int i=0; i<10; i=i+1) { } return 0; }",                     "for" },
    valid_program_case{ "int add(int a, int b) { return a + b; } int main() { return add(1,2); }",  "call" },
    valid_program_case{ "bool f() { return true && false; }",                                       "logical and" },
    valid_program_case{ "bool f() { return true || false; }",                                       "logical or" },
    valid_program_case{ "bool f() { return !true; }",                                               "not" },
    valid_program_case{ "int f() { int x = 0; ++x; return x; }",                                    "prefix inc" },
    valid_program_case{ "int f() { int x = 0; x++; return x; }",                                    "postfix inc" }
));


struct type_error_case {
    std::string_view source_;
    std::string_view description_;
};

class type_error_test : public ::testing::TestWithParam<type_error_case> {};

TEST_P(type_error_test, reports_error) {
    const auto& tc = GetParam();
    semantics_harness h(std::string{ tc.source_ });
    EXPECT_TRUE(h.had_error()) << "Expected error for: " << tc.description_;
}

INSTANTIATE_TEST_SUITE_P(all, type_error_test, ::testing::Values(
    type_error_case{ "int main() { int x = true; return 0; }",                  "bool to int assign" },
    type_error_case{ "int main() { return x; }",                                "undefined var" },
    type_error_case{ "int main() { if (42) { } return 0; }",                    "non-bool condition" },
    type_error_case{ "int main() { int x; int x; return 0; }",                  "redeclaration" },
    type_error_case{ "return 0;",                                               "return outside func" },
    type_error_case{ "int main() { return true; }",                             "return type mismatch" },
    type_error_case{ "int main() { return; }",                                  "missing return value" },
    type_error_case{ "void f() { return 42; }",                                 "return value in void" },
    type_error_case{ "int main() { 42 = 0; return 0; }",                        "assign to literal" },
    type_error_case{ "int main() { return foo(); }",                            "undeclared func" },
    type_error_case{ "int f(int a) { return a; } int g() { return f(1,2); }",   "arg count" },
    type_error_case{ "int f(bool b) { return 0; } int g() { return f(42); }",   "arg type" },
    type_error_case{ "int main() { bool b = !42; return 0; }",                  "not on int" },
    type_error_case{ "int main() { int x; ++true; return 0; }",                 "inc on bool" },
    type_error_case{ "int main() { true + false; return 0; }",                  "arithmetic on bool" }
));

} // namespace tests