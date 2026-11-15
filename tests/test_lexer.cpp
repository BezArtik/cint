#include "core/error/error_codes.hpp"
#include "core/token/token_types.hpp"
#include "pipeline_harness.hpp"

#include <gtest/gtest.h>
#include <ranges>
#include <string_view>
#include <vector>

namespace cint::tests {

using tt = core::token_type;
using err = core::error_code;

void expect_token(const core::token& tok, tt type, std::string_view lexeme = {}) {
    EXPECT_EQ(tok.type_, type) << "Unexpected token type for lexeme '" << tok.lexeme_ << "'";
    if (!lexeme.empty())
        EXPECT_EQ(tok.lexeme_, lexeme) << "Unexpected lexeme for token type " << static_cast<std::uint8_t>(type);
}

struct single_token_case {
    std::string_view source_;
    tt expected_type_;
    std::string_view expected_lexeme_{};
};

struct single_token_test : ::testing::TestWithParam<single_token_case> {};

TEST_P(single_token_test, recognized) {
    auto&& tc = GetParam();
    auto h = pipeline_harness{tc.source_};
    h.lex();
    ASSERT_FALSE(h.had_error());
    ASSERT_GE(h.tokens().size(), 2);
    expect_token(h.tokens()[0], tc.expected_type_, tc.expected_lexeme_);
    EXPECT_EQ(h.tokens()[1].type_, tt::END_OF_FILE);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    ops_and_punct, single_token_test,
    ::testing::Values(
        single_token_case{"(", tt::LEFT_PAREN},
        single_token_case{")", tt::RIGHT_PAREN},
        single_token_case{"{", tt::LEFT_BRACE},
        single_token_case{"}", tt::RIGHT_BRACE},
        single_token_case{",", tt::COMMA},
        single_token_case{".", tt::DOT},
        single_token_case{";", tt::SEMICOLON},

        single_token_case{"+", tt::PLUS},
        single_token_case{"-", tt::MINUS},
        single_token_case{"*", tt::STAR},
        single_token_case{"/", tt::SLASH},
        single_token_case{"%", tt::PERCENT},

        single_token_case{"!", tt::BANG},
        single_token_case{"=", tt::EQUAL},

        single_token_case{"==", tt::EQUAL_EQUAL},
        single_token_case{"!=", tt::BANG_EQUAL},
        single_token_case{"<", tt::LESS},
        single_token_case{"<=", tt::LESS_EQUAL},
        single_token_case{">", tt::GREATER},
        single_token_case{">=", tt::GREATER_EQUAL},

        single_token_case{"++", tt::INCREMENT},
        single_token_case{"--", tt::DECREMENT},

        single_token_case{"+=", tt::PLUS_EQUAL},
        single_token_case{"-=", tt::MINUS_EQUAL},
        single_token_case{"*=", tt::STAR_EQUAL},
        single_token_case{"/=", tt::SLASH_EQUAL},
        single_token_case{"%=", tt::PERCENT_EQUAL},

        single_token_case{"&&", tt::LOGICAL_AND},
        single_token_case{"||", tt::LOGICAL_OR}
));

struct keyword_case {
    std::string_view source_;
    std::string_view lexeme_;
};

struct keyword_test : ::testing::TestWithParam<keyword_case> {};

TEST_P(keyword_test, recognized) {
    auto&& tc = GetParam();
    auto h = pipeline_harness{tc.source_};
    h.lex();
    ASSERT_FALSE(h.had_error());
    ASSERT_GE(h.tokens().size(), 2);
    EXPECT_NE(h.tokens()[0].type_, tt::IDENTIFIER);
    EXPECT_EQ(h.tokens()[0].lexeme_, tc.lexeme_);
    EXPECT_EQ(h.tokens()[1].type_, tt::END_OF_FILE);
}

INSTANTIATE_TEST_SUITE_P(
        all_keywords, keyword_test,
        ::testing::Values(
            keyword_case{"struct", "struct"},
            keyword_case{"if", "if"},
            keyword_case{"else", "else"},
            keyword_case{"while", "while"},
            keyword_case{"for", "for"},
            keyword_case{"return", "return"},
            keyword_case{"let", "let"},
            keyword_case{"fn", "fn"},
            keyword_case{"true", "true"},
            keyword_case{"false", "false"}
));

struct number_case {
    std::string_view source_;
    std::string_view lexeme_;
};

struct number_test : ::testing::TestWithParam<number_case> {};

TEST_P(number_test, recognized) {
    auto&& tc = GetParam();
    auto h = pipeline_harness{tc.source_};
    h.lex();
    ASSERT_FALSE(h.had_error());
    ASSERT_GE(h.tokens().size(), 2);
    expect_token(h.tokens()[0], tt::NUMBER, tc.lexeme_);
    EXPECT_EQ(h.tokens()[1].type_, tt::END_OF_FILE);
}

INSTANTIATE_TEST_SUITE_P(
        numbers, number_test,
        ::testing::Values(
            number_case{"0", "0"},
            number_case{"42", "42"},
            number_case{"999", "999"},
            number_case{"3.14", "3.14"},
            number_case{"0.0", "0.0"}
));

struct bool_case {
    std::string_view source_;
    core::token_type expected_type_;
};

struct bool_test : ::testing::TestWithParam<bool_case> {};

TEST_P(bool_test, recognized) {
    auto&& tc = GetParam();
    auto h = pipeline_harness{tc.source_};
    h.lex();
    ASSERT_FALSE(h.had_error());
    ASSERT_GE(h.tokens().size(), 2);
    expect_token(h.tokens()[0], tc.expected_type_, tc.source_);
    EXPECT_EQ(h.tokens()[1].type_, tt::END_OF_FILE);
}

INSTANTIATE_TEST_SUITE_P(
        bool_literals, bool_test,
        ::testing::Values(
            bool_case{"true", tt::KW_TRUE},
            bool_case{"false", tt::KW_FALSE}));

struct string_case {
    std::string_view source_;
    std::string_view expected_lexeme_;
};

struct string_test : ::testing::TestWithParam<string_case> {};

TEST_P(string_test, recognized) {
    auto&& tc = GetParam();
    auto h = pipeline_harness{tc.source_};
    h.lex();
    ASSERT_FALSE(h.had_error());
    ASSERT_GE(h.tokens().size(), 2);
    expect_token(h.tokens()[0], tt::STRING, tc.expected_lexeme_);
    EXPECT_EQ(h.tokens()[1].type_, tt::END_OF_FILE);
}

INSTANTIATE_TEST_SUITE_P(
        valid_strings, string_test,
        ::testing::Values(
            string_case{"\"hello\"", "\"hello\""},
            string_case{"\"\"", "\"\""},
            string_case{"\"a b c\"", "\"a b c\""},
            string_case{"\"123\"", "\"123\""}));

struct identifier_test : ::testing::TestWithParam<std::string_view> {};

TEST_P(identifier_test, recognized) {
    auto&& id = GetParam();
    auto h = pipeline_harness{id};
    h.lex();
    ASSERT_FALSE(h.had_error());
    ASSERT_GE(h.tokens().size(), 2);
    expect_token(h.tokens()[0], tt::IDENTIFIER, id);
    EXPECT_EQ(h.tokens()[1].type_, tt::END_OF_FILE);
}

INSTANTIATE_TEST_SUITE_P(
        various, identifier_test,
        ::testing::Values("foo", "bar", "_test", "x1", "_", "a123", "my_var"));

struct token_sequence {
    std::string_view source_;
    std::vector<tt> expected_types_;
    std::vector<std::string_view> expected_lexemes_;
};

struct sequence_test : ::testing::TestWithParam<token_sequence> {};

TEST_P(sequence_test, Recognized) {
    auto&& tc = GetParam();
    auto h = pipeline_harness{tc.source_};
    h.lex();
    ASSERT_FALSE(h.had_error());

    ASSERT_EQ(h.tokens().size(), tc.expected_types_.size() + 1)
        << "Unexpected token count for source: " << tc.source_;

    auto&& zip = std::views::zip(h.tokens(), tc.expected_types_, tc.expected_lexemes_);
    for (auto&& [token, type, lexem] : zip) expect_token(token, type, lexem);

    EXPECT_EQ(h.tokens()[tc.expected_types_.size()].type_, tt::END_OF_FILE);
}

INSTANTIATE_TEST_SUITE_P(
    declarations, sequence_test,
    ::testing::Values(
        token_sequence{"let x = 42;",
                       {tt::KW_LET, tt::IDENTIFIER, tt::EQUAL, tt::NUMBER, tt::SEMICOLON},
                       {"let", "x", "=", "42", ";"}},
        token_sequence{"fn add(a, b) { }",
                       {tt::KW_FN, tt::IDENTIFIER, tt::LEFT_PAREN,
                        tt::IDENTIFIER, tt::COMMA, tt::IDENTIFIER,
                        tt::RIGHT_PAREN, tt::LEFT_BRACE, tt::RIGHT_BRACE},
                       {"fn", "add", "(", "a", ",", "b", ")", "{", "}"}},
        token_sequence{"struct Point { x, y }",
                       {tt::KW_STRUCT, tt::IDENTIFIER, tt::LEFT_BRACE,
                        tt::IDENTIFIER, tt::COMMA, tt::IDENTIFIER,
                        tt::RIGHT_BRACE},
                       {"struct", "Point", "{", "x", ",", "y", "}"}}
));
// clang-format on

TEST(lexer_test, line_comment) {
    auto h = pipeline_harness{"42 // comment\n43"};
    h.lex();
    ASSERT_FALSE(h.had_error());
    ASSERT_EQ(h.tokens().size(), 3);
    expect_token(h.tokens()[0], tt::NUMBER, "42");
    expect_token(h.tokens()[1], tt::NUMBER, "43");
    EXPECT_EQ(h.tokens()[2].type_, tt::END_OF_FILE);
}

TEST(lexer_test, unterminated_string) {
    auto h = pipeline_harness{"\"unterminated"};
    h.lex();
    if (!h.had_error(err::unterminated_string)) h.render_diagnostics();
    EXPECT_TRUE(h.had_error(err::unterminated_string));
}

TEST(lexer_test, unexpected_character) {
    auto h = pipeline_harness{"@"};
    h.lex();
    if (!h.had_error(err::unexpected_character)) h.render_diagnostics();
    EXPECT_TRUE(h.had_error(err::unexpected_character));
}

TEST(lexer_test, unexpected_character_recovery) {
    auto h = pipeline_harness{"42 @ 43"};
    h.lex();
    if (!h.had_error(err::unexpected_character)) h.render_diagnostics();
    EXPECT_TRUE(h.had_error(err::unexpected_character));
    ASSERT_GE(h.tokens().size(), 3);
    expect_token(h.tokens()[0], tt::NUMBER, "42");
    expect_token(h.tokens()[1], tt::NUMBER, "43");
}

}  // namespace cint::tests
