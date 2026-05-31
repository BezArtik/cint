#include <gtest/gtest.h>
#include "lexer/lexer.hpp"
#include "core/error/error_report.hpp"
#include "core/token/token_types.hpp"
#include <vector>
#include <string>
#include <tuple>

namespace {

class lexer_harness {
public:
    lexer_harness(std::string source)
        : source_code_(std::move(source))
        , reporter_(source_code_)
        , lex_(source_code_, reporter_) {
        tokens_ = lex_.scan_tokens();
    }

    size_t size() const noexcept { return tokens_.size(); }
    const core::token& operator[](size_t i) const noexcept { return tokens_[i]; }
    bool had_error() const noexcept { return reporter_.has_error(); }

private:
    std::string source_code_;
    core::error_reporter reporter_;
    lexer::lexer lex_;
    std::vector<core::token> tokens_;
};

void expect_token(const core::token& tok, core::token_type type,
    std::string_view lexeme = {}) {
    EXPECT_EQ(tok.type_, type) << "Unexpected token type for lexeme '" << tok.lexeme_ << "'";
    if (!lexeme.empty()) {
        EXPECT_EQ(tok.lexeme_, lexeme) << "Unexpected lexeme for token type "
            << static_cast<int>(type);
    }
}

void expect_eof(const lexer_harness& h, size_t index) {
    ASSERT_LT(index, h.size());
    EXPECT_EQ(h[index].type_, core::token_type::END_OF_FILE);
}

struct single_token_case {
    std::string_view source_;
    core::token_type expected_type_;
    std::string_view expected_lexeme_;
};

class single_token_test : public ::testing::TestWithParam<single_token_case> {};

TEST_P(single_token_test, recognized) {
    const auto& tc = GetParam();
    lexer_harness h(std::string{ tc.source_ });
    ASSERT_GE(h.size(), 2) << "Expected at least token + EOF for source: " << tc.source_;
    expect_token(h[0], tc.expected_type_, tc.expected_lexeme_);
    expect_eof(h, 1);
}

INSTANTIATE_TEST_SUITE_P(ops_and_punct, single_token_test, ::testing::Values(
    single_token_case{ "(",  core::token_type::LEFT_PAREN },
    single_token_case{ ")",  core::token_type::RIGHT_PAREN },
    single_token_case{ "{",  core::token_type::LEFT_BRACE },
    single_token_case{ "}",  core::token_type::RIGHT_BRACE },
    single_token_case{ ",",  core::token_type::COMMA },
    single_token_case{ ".",  core::token_type::DOT },
    single_token_case{ ";",  core::token_type::SEMICOLON },

    single_token_case{ "+",  core::token_type::PLUS },
    single_token_case{ "-",  core::token_type::MINUS },
    single_token_case{ "*",  core::token_type::STAR },
    single_token_case{ "/",  core::token_type::SLASH },
    single_token_case{ "%",  core::token_type::PERCENT },

    single_token_case{ "!",  core::token_type::BANG },
    single_token_case{ "=",  core::token_type::EQUAL },

    single_token_case{ "==", core::token_type::EQUAL_EQUAL },
    single_token_case{ "!=", core::token_type::BANG_EQUAL },
    single_token_case{ "<",  core::token_type::LESS },
    single_token_case{ "<=", core::token_type::LESS_EQUAL },
    single_token_case{ ">",  core::token_type::GREATER },
    single_token_case{ ">=", core::token_type::GREATER_EQUAL },

    single_token_case{ "++", core::token_type::INCREMENT },
    single_token_case{ "--", core::token_type::DECREMENT },

    single_token_case{ "+=", core::token_type::PLUS_EQUAL },
    single_token_case{ "-=", core::token_type::MINUS_EQUAL },
    single_token_case{ "*=", core::token_type::STAR_EQUAL },
    single_token_case{ "/=", core::token_type::SLASH_EQUAL },
    single_token_case{ "%=", core::token_type::PERCENT_EQUAL },

    single_token_case{ "&&", core::token_type::AND },
    single_token_case{ "||", core::token_type::OR }
));


struct keyword_case {
    std::string_view source_;
    std::string_view lexeme_;
};

class keyword_test : public ::testing::TestWithParam<keyword_case> {};

TEST_P(keyword_test, recognized) {
    const auto& tc = GetParam();
    lexer_harness h(std::string{ tc.source_ });
    ASSERT_GE(h.size(), 2);
    expect_token(h[0], core::token_type::KEYWORD, tc.lexeme_);
    EXPECT_TRUE(h[0].is_keyword());
    expect_eof(h, 1);
}

INSTANTIATE_TEST_SUITE_P(all_keywords, keyword_test, ::testing::Values(
    keyword_case{ "int",    "int" },
    keyword_case{ "double", "double" },
    keyword_case{ "bool",   "bool" },
    keyword_case{ "string", "string" },
    keyword_case{ "void",   "void" },
    keyword_case{ "if",     "if" },
    keyword_case{ "else",   "else" },
    keyword_case{ "while",  "while" },
    keyword_case{ "for",    "for" },
    keyword_case{ "return", "return" },
    keyword_case{ "true",   "true" },
    keyword_case{ "false",  "false" }
));


struct number_case {
    std::string_view source_;
    std::string_view lexeme_;
    bool is_double_;
};

class number_test : public ::testing::TestWithParam<number_case> {};

TEST_P(number_test, recognized) {
    const auto& tc = GetParam();
    lexer_harness h(std::string{ tc.source_ });
    ASSERT_GE(h.size(), 2);
    expect_token(h[0], core::token_type::NUMBER, tc.lexeme_);
    EXPECT_EQ(h[0].is_double_literal(), tc.is_double_);
    expect_eof(h, 1);
}

INSTANTIATE_TEST_SUITE_P(integers, number_test, ::testing::Values(
    number_case{ "0",    "0",    false },
    number_case{ "42",   "42",   false },
    number_case{ "999",  "999",  false }
));

INSTANTIATE_TEST_SUITE_P(doubles, number_test, ::testing::Values(
    number_case{ "3.14",  "3.14",  true },
    number_case{ "0.0",   "0.0",   true },
    number_case{ "1.5",   "1.5",   true }
));


struct string_case {
    std::string_view source_;
    std::string_view expected_lexeme_;
};

class string_test : public ::testing::TestWithParam<string_case> {};

TEST_P(string_test, recognized) {
    const auto& tc = GetParam();
    lexer_harness h(std::string{ tc.source_ });
    ASSERT_GE(h.size(), 2);
    expect_token(h[0], core::token_type::STRING, tc.expected_lexeme_);
    EXPECT_TRUE(h[0].is_string_literal());
    expect_eof(h, 1);
}

INSTANTIATE_TEST_SUITE_P(valid_strings, string_test, ::testing::Values(
    string_case{ "\"hello\"",        "\"hello\"" },
    string_case{ "\"\"",             "\"\"" },
    string_case{ "\"a b c\"",        "\"a b c\"" },
    string_case{ "\"123\"",          "\"123\"" }
));

class identifier_test : public ::testing::TestWithParam<std::string_view> {};

TEST_P(identifier_test, recognized) {
    auto id = GetParam();
    lexer_harness h(std::string{ id });
    ASSERT_GE(h.size(), 2);
    expect_token(h[0], core::token_type::IDENTIFIER, id);
    EXPECT_TRUE(h[0].is_identifier());
    expect_eof(h, 1);
}

INSTANTIATE_TEST_SUITE_P(various, identifier_test, ::testing::Values(
    "foo", "bar", "_test", "x1", "_", "a123", "my_var"
));


struct token_sequence {
    std::string_view source_;
    std::vector<core::token_type> expected_types_;
    std::vector<std::string_view> expected_lexemes_;
};

class sequence_test : public ::testing::TestWithParam<token_sequence> {};

TEST_P(sequence_test, Recognized) {
    const auto& tc = GetParam();
    lexer_harness h(std::string{ tc.source_ });

    ASSERT_EQ(h.size(), tc.expected_types_.size() + 1)
        << "Unexpected token count for source: " << tc.source_;

    for (size_t i = 0; i < tc.expected_types_.size(); ++i) {
        expect_token(h[i], tc.expected_types_[i],
            i < tc.expected_lexemes_.size() ? tc.expected_lexemes_[i] : std::string_view{});
    }
    expect_eof(h, tc.expected_types_.size());
}

INSTANTIATE_TEST_SUITE_P(declarations, sequence_test, ::testing::Values(
    token_sequence{
        "int x = 42;",
        {core::token_type::KEYWORD, core::token_type::IDENTIFIER,
            core::token_type::EQUAL,   core::token_type::NUMBER,
            core::token_type::SEMICOLON},
        {"int", "x", "=", "42", ";"}
    },
    token_sequence{
        "double pi = 3.14;",
        {core::token_type::KEYWORD, core::token_type::IDENTIFIER,
            core::token_type::EQUAL,   core::token_type::NUMBER,
            core::token_type::SEMICOLON},
        {"double", "pi", "=", "3.14", ";"}
    },
    token_sequence{
        "if (x < 10) { }",
        {core::token_type::KEYWORD,   core::token_type::LEFT_PAREN,
            core::token_type::IDENTIFIER,core::token_type::LESS,
            core::token_type::NUMBER,    core::token_type::RIGHT_PAREN,
            core::token_type::LEFT_BRACE,core::token_type::RIGHT_BRACE},
        {"if", "(", "x", "<", "10", ")", "{", "}"}
    }
));

TEST(lexer_test, line_comment) {
    lexer_harness h("42 // comment\n43");
    ASSERT_EQ(h.size(), 3);
    expect_token(h[0], core::token_type::NUMBER, "42");
    expect_token(h[1], core::token_type::NUMBER, "43");
    expect_eof(h, 2);
}

TEST(lexer_test, line_numbers) {
    lexer_harness h("int\nx\n=");
    EXPECT_EQ(h[0].line_, 1);
    EXPECT_EQ(h[1].line_, 2);
    EXPECT_EQ(h[2].line_, 3);
}

TEST(lexer_test, column_numbers) {
    lexer_harness h("int x");
    EXPECT_EQ(h[0].column_, 4);
    EXPECT_EQ(h[1].column_, 6);
}

TEST(LexerTest, unterminated_string) {
    lexer_harness h("\"unterminated");
    EXPECT_TRUE(h.had_error());
}

TEST(lexer_test, unexpected_character) {
    lexer_harness h("@");
    EXPECT_TRUE(h.had_error());
}

} // anonymous namespace