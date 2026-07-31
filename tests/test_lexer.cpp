// tests/test_lexer.cpp

#include "core/token/token_types.hpp"
#include "pipeline_harness.hpp"

#include <gtest/gtest.h>
#include <string>
#include <string_view>
#include <vector>

namespace tests {

using tt = core::token_type;

void expect_token(const core::token& tok, tt type, std::string_view lexeme = {}) {
    EXPECT_EQ(tok.type_, type) << "Unexpected token type for lexeme '" << tok.lexeme_ << "'";
    if (!lexeme.empty()) {
        EXPECT_EQ(tok.lexeme_, lexeme) << "Unexpected lexeme for token type " << static_cast<uint8_t>(type);
    }
}

struct single_token_case {
    std::string_view source_;
    tt expected_type_;
    std::string_view expected_lexeme_{};
};

class single_token_test : public ::testing::TestWithParam<single_token_case> {};

TEST_P(single_token_test, recognized) {
    auto&& tc = GetParam();
    pipeline_harness h{tc.source_};
    ASSERT_TRUE(h.lex());
    ASSERT_GE(h.tokens().size(), 2) << "Expected at least token + EOF for source: " << tc.source_;
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

class keyword_test : public ::testing::TestWithParam<keyword_case> {};

TEST_P(keyword_test, recognized) {
    auto&& tc = GetParam();
    pipeline_harness h{tc.source_};
    ASSERT_TRUE(h.lex());
    ASSERT_GE(h.tokens().size(), 2);
    EXPECT_NE(h.tokens()[0].type_, tt::IDENTIFIER);
    EXPECT_EQ(h.tokens()[0].lexeme_, tc.lexeme_);
    EXPECT_EQ(h.tokens()[1].type_, tt::END_OF_FILE);
}

INSTANTIATE_TEST_SUITE_P(
        all_keywords, keyword_test,
        ::testing::Values(
            keyword_case{"int", "int"}, 
            keyword_case{"double", "double"},
            keyword_case{"bool", "bool"}, 
            keyword_case{"string", "string"},
            keyword_case{"struct", "struct"}, 
            keyword_case{"void", "void"},
            keyword_case{"if", "if"}, 
            keyword_case{"else", "else"},
            keyword_case{"while", "while"}, 
            keyword_case{"for", "for"},
            keyword_case{"return", "return"}, 
            keyword_case{"true", "true"},
            keyword_case{"false", "false"}
));

struct number_case {
    std::string_view source_;
    std::string_view lexeme_;
    bool is_double_;
};

class number_test : public ::testing::TestWithParam<number_case> {};

TEST_P(number_test, recognized) {
    auto&& tc = GetParam();
    pipeline_harness h{tc.source_};
    ASSERT_TRUE(h.lex());
    ASSERT_GE(h.tokens().size(), 2);
    expect_token(h.tokens()[0], tt::NUMBER, tc.lexeme_);

    ASSERT_TRUE(h.tokens()[0].literal_value_);
    if (tc.is_double_) {
        EXPECT_TRUE(h.tokens()[0].literal_value_->is_double());
        EXPECT_DOUBLE_EQ(h.tokens()[0].literal_value_->to_double(), std::stod(std::string(tc.lexeme_)));
    } else {
        EXPECT_TRUE(h.tokens()[0].literal_value_->is_int());
        EXPECT_EQ(h.tokens()[0].literal_value_->to_int(), std::stoll(std::string(tc.lexeme_)));
    }
    EXPECT_EQ(h.tokens()[1].type_, tt::END_OF_FILE);
}

INSTANTIATE_TEST_SUITE_P(
        integers, number_test,
        ::testing::Values(
            number_case{"0", "0", false}, 
            number_case{"42", "42", false},
            number_case{"999", "999", false}
));

INSTANTIATE_TEST_SUITE_P(
        doubles, number_test,
        ::testing::Values(
            number_case{"3.14", "3.14", true}, 
            number_case{"0.0", "0.0", true},
            number_case{"1.5", "1.5", true}
));

struct bool_case {
    std::string_view source_;
    core::token_type expected_type_;
    bool expected_value_;
};

class bool_test : public ::testing::TestWithParam<bool_case> {};

TEST_P(bool_test, recognized) {
    auto&& tc = GetParam();
    pipeline_harness h{tc.source_};
    ASSERT_TRUE(h.lex());
    ASSERT_GE(h.tokens().size(), 2);
    expect_token(h.tokens()[0], tc.expected_type_, tc.source_);
    ASSERT_TRUE(h.tokens()[0].literal_value_.has_value());
    EXPECT_EQ(h.tokens()[0].literal_value_->to_bool(), tc.expected_value_);
    EXPECT_EQ(h.tokens()[1].type_, tt::END_OF_FILE);
}

INSTANTIATE_TEST_SUITE_P(
        bool_literals, bool_test,
        ::testing::Values(
            bool_case{"true", tt::KW_TRUE, true},
            bool_case{"false", tt::KW_FALSE, false}));

struct string_case {
    std::string_view source_;
    std::string_view expected_lexeme_;
};

class string_test : public ::testing::TestWithParam<string_case> {};

TEST_P(string_test, recognized) {
    auto&& tc = GetParam();
    pipeline_harness h{tc.source_};
    ASSERT_TRUE(h.lex());
    ASSERT_GE(h.tokens().size(), 2);
    expect_token(h.tokens()[0], tt::STRING, tc.expected_lexeme_);
    EXPECT_TRUE(h.tokens()[0].literal_value_);
    EXPECT_EQ(h.tokens()[1].type_, tt::END_OF_FILE);
}

INSTANTIATE_TEST_SUITE_P(
        valid_strings, string_test,
        ::testing::Values(
            string_case{"\"hello\"", "\"hello\""}, 
            string_case{"\"\"", "\"\""},
            string_case{"\"a b c\"", "\"a b c\""}, 
            string_case{"\"123\"", "\"123\""}));

class identifier_test : public ::testing::TestWithParam<std::string_view> {};

TEST_P(identifier_test, recognized) {
    auto&& id = GetParam();
    pipeline_harness h{id};
    ASSERT_TRUE(h.lex());
    ASSERT_GE(h.tokens().size(), 2);
    expect_token(h.tokens()[0], tt::IDENTIFIER, id);
    EXPECT_FALSE(h.tokens()[0].literal_value_);
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

class sequence_test : public ::testing::TestWithParam<token_sequence> {};

TEST_P(sequence_test, Recognized) {
    auto&& tc = GetParam();
    pipeline_harness h{tc.source_};
    ASSERT_TRUE(h.lex());

    ASSERT_EQ(h.tokens().size(), tc.expected_types_.size() + 1) << "Unexpected token count for source: " << tc.source_;

    for (size_t i = 0; i < tc.expected_types_.size(); ++i) {
        expect_token(h.tokens()[i], tc.expected_types_[i],
                     i < tc.expected_lexemes_.size() ? tc.expected_lexemes_[i] : std::string_view{});
    }
    EXPECT_EQ(h.tokens()[tc.expected_types_.size()].type_, tt::END_OF_FILE);
}

INSTANTIATE_TEST_SUITE_P(
    declarations, sequence_test,
    ::testing::Values(
        token_sequence{"int x = 42;",
                       {tt::KW_INT, tt::IDENTIFIER, tt::EQUAL, tt::NUMBER, tt::SEMICOLON},
                       {"int", "x", "=", "42", ";"}},
        token_sequence{"double pi = 3.14;",
                       {tt::KW_DOUBLE, tt::IDENTIFIER, tt::EQUAL, tt::NUMBER, tt::SEMICOLON},
                       {"double", "pi", "=", "3.14", ";"}},
        token_sequence{"if (x < 10) { }",
                       {tt::KW_IF, tt::LEFT_PAREN, tt::IDENTIFIER, tt::LESS, tt::NUMBER,
                        tt::RIGHT_PAREN, tt::LEFT_BRACE, tt::RIGHT_BRACE},
                       {"if", "(", "x", "<", "10", ")", "{", "}"}}));
// clang-format on

TEST(lexer_test, line_comment) {
    pipeline_harness h{"42 // comment\n43"};
    ASSERT_TRUE(h.lex());
    ASSERT_EQ(h.tokens().size(), 3);
    expect_token(h.tokens()[0], tt::NUMBER, "42");
    expect_token(h.tokens()[1], tt::NUMBER, "43");
    EXPECT_EQ(h.tokens()[2].type_, tt::END_OF_FILE);
}

TEST(LexerTest, unterminated_string) {
    pipeline_harness h{"\"unterminated"};
    h.lex();
    EXPECT_TRUE(h.had_error());
}

TEST(lexer_test, unexpected_character) {
    pipeline_harness h{"@"};
    h.lex();
    EXPECT_TRUE(h.had_error());
}

}  // namespace tests
