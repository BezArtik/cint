// parser/parser.cpp

#include "parser/parser.hpp"

#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/error/error_codes.hpp"
#include "core/token/keywords.hpp"
#include "core/token/token_types.hpp"
#include "core/utils/arena.hpp"

#include <algorithm>
#include <cassert>
#include <utility>
#include <vector>

namespace parser {

using tt = core::token_type;
using err = core::error_code;

namespace {

struct infix_rule {
    tt type_;
    int8_t precedence_;
    bool right_assoc_;
};

constexpr std::array infix_table = {
    infix_rule{tt::LOGICAL_OR, 2, false},  infix_rule{tt::LOGICAL_AND, 3, false},

    infix_rule{tt::BIT_OR, 4, false},      infix_rule{tt::XOR, 5, false},
    infix_rule{tt::BIT_AND, 6, false},

    infix_rule{tt::EQUAL_EQUAL, 7, false}, infix_rule{tt::BANG_EQUAL, 7, false},
    infix_rule{tt::LESS, 8, false},        infix_rule{tt::LESS_EQUAL, 8, false},
    infix_rule{tt::GREATER, 8, false},     infix_rule{tt::GREATER_EQUAL, 8, false},

    infix_rule{tt::SHL, 9, false},         infix_rule{tt::SHR, 9, false},

    infix_rule{tt::PLUS, 10, false},       infix_rule{tt::MINUS, 10, false},

    infix_rule{tt::STAR, 11, false},       infix_rule{tt::SLASH, 11, false},
    infix_rule{tt::PERCENT, 11, false},
};

int8_t get_precedence(tt type) {
    auto it = std::ranges::find(infix_table, type, &infix_rule::type_);
    return it != infix_table.end() ? it->precedence_ : -1;
}

bool is_right_assoc(tt type) {
    auto it = std::ranges::find(infix_table, type, &infix_rule::type_);
    return it != infix_table.end() ? it->right_assoc_ : false;
}

bool is_assignment(tt type) noexcept {
    return type == tt::EQUAL || type == tt::PLUS_EQUAL || type == tt::MINUS_EQUAL || type == tt::STAR_EQUAL ||
           type == tt::SLASH_EQUAL || type == tt::PERCENT_EQUAL || type == tt::BIT_AND_EQUAL ||
           type == tt::BIT_OR_EQUAL || type == tt::XOR_EQUAL || type == tt::SHL_EQUAL || type == tt::SHR_EQUAL;
}

}  // namespace

parser::parser(const std::pmr::vector<core::token>& tokens, core::error_reporter& reporter, core::arena& arena,
               core::arena_memory_resource& mr)
    : tokens_(tokens), reporter_(reporter), arena_(arena), mr_(mr) {}

std::vector<ast::stmt_ptr> parser::parse() {
    std::pmr::vector<ast::stmt_ptr> statements(&mr_);
    while (!is_at_end()) {
        auto stmt = declaration();
        if (stmt) statements.push_back(std::move(stmt));
    }
    return {std::make_move_iterator(statements.begin()), std::make_move_iterator(statements.end())};
}

bool parser::match(std::initializer_list<tt> types) noexcept {
    for (auto type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

const core::token& parser::consume(tt type, err code) {
    if (check(type)) return advance();
    reporter_.parse_error(peek(), code);
}

bool parser::check(tt type) const noexcept {
    if (is_at_end()) return false;
    return peek().type_ == type;
}

const core::token& parser::advance() noexcept {
    if (!is_at_end()) current_++;
    return prev();
}

bool parser::is_at_end() const noexcept {
    return peek().type_ == tt::END_OF_FILE;
}
const core::token& parser::peek() const noexcept {
    return tokens_[current_];
}
const core::token& parser::prev() const noexcept {
    assert(current_ > 0 && "No previous token available");
    return tokens_[current_ - 1];
}

ast::stmt_ptr parser::declaration() {
    try {
        if (match({tt::KEYWORD})) {
            auto kw = prev().as_keyword();

            if (!kw || !kw->is_type_) {
                current_--;
                return statement();
            }

            const auto& type = kw->semantic_type_;
            const auto& name = consume(tt::IDENTIFIER, err::expected_identifier);

            if (match({tt::LEFT_PAREN})) {
                return func_declaration(type, name);
            } else {
                return var_declaration(type, name);
            }
        }
        return statement();
    } catch (const core::parse_error&) {
        synchronize();
        return nullptr;
    }
}

ast::stmt_ptr parser::var_declaration(core::type type, const core::token& name) {
    if (match({tt::LEFT_BRACKET})) {
        size_t array_size = 0;

        if (match({tt::NUMBER})) {
            auto size_token = prev();
            auto lex = size_token.lexeme_;
            auto [ptr, ec] = std::from_chars(lex.data(), lex.data() + lex.size(), array_size);
            if (ec != std::errc{} || ptr != lex.data() + lex.size() || array_size == 0)
                reporter_.parse_error(size_token, err::unexpected_token);
        } else if (!check(tt::RIGHT_BRACKET)) {
            reporter_.parse_error(peek(), err::expected_right_bracket);
        }

        consume(tt::RIGHT_BRACKET, err::expected_right_bracket);
        type = core::type::array_type(type, array_size);
    }

    std::optional<ast::expression> initializer;
    if (match({tt::EQUAL})) initializer = match({tt::LEFT_BRACE}) ? array_literal() : expression();
    consume(tt::SEMICOLON, err::expected_semicolon);
    return ast::make_stmt<ast::var_declaration>(arena_, name, std::move(type), name, std::move(initializer));
}

ast::stmt_ptr parser::func_declaration(core::type return_type, const core::token& name) {
    ast::func_declaration func(return_type, name);

    if (!check(tt::RIGHT_PAREN)) {
        do { func.params_.push_back(parse_param()); } while (match({tt::COMMA}));
    }

    consume(tt::RIGHT_PAREN, err::expected_right_paren);
    consume(tt::LEFT_BRACE, err::expected_left_brace);

    auto body = block_statement();
    auto& block = std::get<ast::block_stmt>(body->data_);
    auto* body_copy = arena_.allocate<ast::block_stmt>(std::move(block));
    func.body_ = core::arena_ptr<ast::block_stmt>(body_copy);

    return ast::make_stmt(arena_, std::move(func));
}

ast::func_param parser::parse_param() {
    if (!match({tt::KEYWORD})) reporter_.parse_error(peek(), err::expected_type);

    auto kw = prev().as_keyword();
    if (!kw || !kw->is_type_ || kw->semantic_type_.is_void()) reporter_.parse_error(prev(), err::expected_type);

    const auto& type = kw->semantic_type_;
    const auto& name = consume(tt::IDENTIFIER, err::expected_identifier);
    return {type, name};
}

ast::stmt_ptr parser::statement() {
    if (match({tt::KEYWORD})) {
        auto kw = prev().as_keyword();
        if (!kw) reporter_.parse_error(prev(), err::unexpected_token);

        const auto& lex = kw->lexeme_;

        if (lex == "while") return while_statement();
        if (lex == "for") return for_statement();
        if (lex == "if") return if_statement();
        if (lex == "return") return return_statement();

        current_--;
    }

    if (match({tt::LEFT_BRACE})) return block_statement();

    auto expr = expression();
    consume(tt::SEMICOLON, err::expected_semicolon);

    return ast::make_stmt<ast::expression_stmt>(arena_, prev(), std::move(expr));
}

ast::stmt_ptr parser::while_statement() {
    consume(tt::LEFT_PAREN, err::expected_left_paren_while);
    auto condition = expression();
    consume(tt::RIGHT_PAREN, err::expected_right_paren_condition);

    auto body = statement();
    return ast::make_stmt<ast::while_stmt>(arena_, prev(), std::move(condition), std::move(body));
}

ast::stmt_ptr parser::for_statement() {
    consume(tt::LEFT_PAREN, err::expected_left_paren_for);

    ast::stmt_ptr initializer;
    if (match({tt::SEMICOLON})) {
    } else if (match({tt::KEYWORD})) {
        const auto& kw = prev().as_keyword();
        if (kw && kw->is_type_) {
            const auto& type = kw->semantic_type_;
            initializer = var_declaration(type, consume(tt::IDENTIFIER, err::expected_identifier));
        } else {
            current_--;
            initializer = statement();
        }
    } else {
        initializer = statement();
    }

    std::optional<ast::expression> condition;
    if (!check(tt::SEMICOLON)) condition = expression();
    consume(tt::SEMICOLON, err::expected_semicolon);

    std::optional<ast::expression> increment;
    if (!check(tt::RIGHT_PAREN)) increment = expression();
    consume(tt::RIGHT_PAREN, err::expected_right_paren);

    auto body = statement();
    return ast::make_stmt<ast::for_stmt>(arena_, prev(), std::move(initializer), std::move(condition),
                                         std::move(increment), std::move(body));
}

ast::stmt_ptr parser::if_statement() {
    consume(tt::LEFT_PAREN, err::expected_left_paren_if);
    auto condition = expression();
    consume(tt::RIGHT_PAREN, err::expected_right_paren_condition);

    auto then_branch = statement();
    ast::stmt_ptr else_branch;

    if (match({tt::KEYWORD})) {
        auto kw = prev().as_keyword();
        if (kw && kw->lexeme_ == "else") {
            else_branch = statement();
        } else {
            current_--;
        }
    }

    return ast::make_stmt<ast::if_stmt>(arena_, prev(), std::move(condition), std::move(then_branch),
                                        std::move(else_branch));
}

ast::stmt_ptr parser::return_statement() {
    const auto& keyword = prev();

    std::optional<ast::expression> value;
    if (!check(tt::SEMICOLON)) value = expression();
    consume(tt::SEMICOLON, err::expected_semicolon);

    return ast::make_stmt<ast::return_stmt>(arena_, keyword, keyword, std::move(value));
}

ast::stmt_ptr parser::block_statement() {
    ast::stmt_list statements(&mr_);
    while (!check(tt::RIGHT_BRACE) && !is_at_end()) {
        auto stmt = declaration();
        if (stmt) statements.push_back(std::move(stmt));
    }
    consume(tt::RIGHT_BRACE, err::expected_right_brace);
    const auto& brace = prev();
    return ast::make_stmt<ast::block_stmt>(arena_, brace, std::move(statements));
}

ast::expression parser::expression() {
    return assignment();
}

ast::expression parser::assignment() {
    auto left = parse_expression(0);

    if (is_assignment(peek().type_)) {
        auto op = advance();
        auto right = assignment();
        return ast::make_expr<ast::assignment_expr>(arena_, op, std::move(left), op, std::move(right));
    }

    return left;
}

ast::expression parser::parse_expression(int8_t precedence) {
    auto left = unary();

    while (true) {
        auto op_type = peek().type_;
        auto p = get_precedence(op_type);
        if (p < precedence) break;

        auto op = advance();
        auto next_prec = is_right_assoc(op_type) ? p : p + 1;
        auto right = parse_expression(next_prec);
        left = ast::make_expr<ast::binary_expr>(arena_, op, std::move(left), op, std::move(right));
    }

    return left;
}

ast::expression parser::unary() {
    if (match({tt::BANG, tt::MINUS, tt::INCREMENT, tt::DECREMENT})) {
        const auto& op = prev();
        auto operand = unary();
        return ast::make_expr<ast::unary_expr>(arena_, op, op, std::move(operand));
    }
    return postfix();
}

ast::expression parser::postfix() {
    auto expr = primary();

    while (true) {
        if (match({tt::LEFT_BRACKET})) {
            expr = finish_index(std::move(expr));
        } else if (match({tt::INCREMENT, tt::DECREMENT})) {
            const auto& op = prev();
            expr = ast::make_expr<ast::postfix_expr>(arena_, op, std::move(expr), op);
        } else {
            break;
        }
    }

    return expr;
}

ast::expression parser::array_literal() {
    ast::expr_list elements(&mr_);
    const auto& brace = prev();
    if (!check(tt::RIGHT_BRACE)) {
        do { elements.push_back(expression()); } while (match({tt::COMMA}));
    }
    consume(tt::RIGHT_BRACE, err::expected_right_brace);
    return ast::make_expr<ast::array_literal_expr>(arena_, brace, std::move(elements));
}

ast::expression parser::primary() {
    if (match({tt::NUMBER, tt::STRING})) return ast::make_expr_val<ast::literal_expr>(prev());

    if (match({tt::KEYWORD})) {
        auto kw = prev().as_keyword();
        if (kw && (kw->lexeme_ == "true" || kw->lexeme_ == "false")) {
            return ast::make_expr_val<ast::literal_expr>(prev());
        }
        reporter_.parse_error(prev(), err::expected_expression);
    }

    if (match({tt::IDENTIFIER})) {
        const auto& name = prev();

        if (match({tt::LEFT_PAREN})) return finish_call(name);
        return ast::make_expr_val<ast::variable_expr>(name);
    }

    if (match({tt::LEFT_PAREN})) {
        auto expr = expression();
        consume(tt::RIGHT_PAREN, err::expected_right_paren);
        return expr;
    }

    if (match({tt::LEFT_BRACE})) return array_literal();

    reporter_.parse_error(peek(), err::expected_expression);
}

ast::expression parser::finish_call(const core::token& callee) {
    ast::expr_list args(&mr_);

    if (!check(tt::RIGHT_PAREN)) {
        do { args.push_back(expression()); } while (match({tt::COMMA}));
    }

    consume(tt::RIGHT_PAREN, err::expected_right_paren);

    return ast::make_expr<ast::call_expr>(arena_, callee, callee, std::move(args));
}

ast::expression parser::finish_index(ast::expression object) {
    const auto& bracket = prev();
    auto index = expression();
    consume(tt::RIGHT_BRACKET, err::expected_right_bracket);
    return ast::make_expr<ast::index_expr>(arena_, bracket, std::move(object), std::move(index));
}

void parser::synchronize() {
    advance();

    while (!is_at_end()) {
        if (prev().type_ == tt::SEMICOLON) return;
        auto kw = peek().as_keyword();
        if (kw && kw->can_start_statement_) return;
        advance();
    }
}

}  // namespace parser
