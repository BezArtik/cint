// parser/parser.cpp

#include "parser/parser.hpp"
#include "core/token/token_types.hpp"
#include "core/token/keywords.hpp"
#include "core/error/error_codes.hpp"
#include <stdexcept>
#include <string>
#include <utility>
#include <algorithm>

namespace parser {

using tt = core::token_type;
using err = core::error_code;

parser::parser(const std::vector<core::token>& tokens, core::error_reporter& reporter)
    : tokens_(tokens), reporter_(reporter) {
}

std::vector<ast::stmt_ptr> parser::parse() {
    std::vector<ast::stmt_ptr> statements_;
    while (!is_at_end()) {
        auto stmt = declaration();
        if (stmt) statements_.push_back(std::move(stmt));
    }
    return statements_;
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

bool parser::is_at_end() const noexcept { return peek().type_ == tt::END_OF_FILE; }
const core::token& parser::peek() const noexcept { return tokens_[current_]; }
const core::token& parser::prev() const noexcept { return tokens_[current_ - 1]; }

ast::stmt_ptr parser::declaration() {
    try {
        if (match({ tt::KEYWORD })) {
            auto kw = prev().as_keyword();

            if (!kw || !kw->is_type_) {
                current_--;
                return statement();
            }

            const auto& type = kw->semantic_type_;
            const auto& name = consume(tt::IDENTIFIER, err::expected_identifier);

            if (match({ tt::LEFT_PAREN })) {
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
    if (match({ tt::LEFT_BRACKET })) {
        if (!check(tt::RIGHT_BRACKET)) {
			reporter_.parse_error(peek(), err::expected_right_bracket);
        }
        consume(tt::RIGHT_BRACKET, err::expected_right_bracket);
		type = core::type::array_type(type, 0);
    }

    std::optional<ast::expression> initializer;
    if (match({ tt::EQUAL })) {
        if (match({ tt::LEFT_BRACE })) {
            initializer = array_literal();
        } else {
            initializer = expression();
        }
    }
    consume(tt::SEMICOLON, err::expected_semicolon);
    return make_stmt<ast::var_declaration>(name, std::move(type), name, std::move(initializer));
}

ast::stmt_ptr parser::func_declaration(core::type return_type, const core::token& name) {
    ast::func_declaration func(return_type, name);

    if (!check(tt::RIGHT_PAREN)) {
        do {
            func.params_.push_back(parse_param());
        } while (match({ tt::COMMA }));
    }

    consume(tt::RIGHT_PAREN, err::expected_right_paren);
    consume(tt::LEFT_BRACE, err::expected_left_brace);

    auto body = block_statement();
    auto& block = std::get<ast::block_stmt>(body->data_);
    func.body_ = std::make_unique<ast::block_stmt>(std::move(block));

    return make_stmt(std::move(func));
}

ast::func_param parser::parse_param() {
    if (!match({ tt::KEYWORD })) {
        reporter_.parse_error(peek(), err::expected_type);
    }

    auto kw = prev().as_keyword();
    if (!kw || !kw->is_type_ || kw->semantic_type_.is_void()) {
        reporter_.parse_error(prev(), err::expected_type);
    }

    const auto& type = kw->semantic_type_;
    const auto& name = consume(tt::IDENTIFIER, err::expected_identifier);
    return { type, name };
}

ast::stmt_ptr parser::statement() {
    if (match({ tt::KEYWORD })) {
        auto kw = prev().as_keyword();
        if (!kw) reporter_.parse_error(prev(), err::unexpected_token);

        const auto& lex = kw->lexeme_;

        if (lex == "while")  return while_statement();
        if (lex == "for")    return for_statement();
        if (lex == "if")     return if_statement();
        if (lex == "return") return return_statement();

        current_--;
    }

    if (match({ tt::LEFT_BRACE })) return block_statement();
        
    auto expr = expression();
    consume(tt::SEMICOLON, err::expected_semicolon);

    return make_stmt<ast::expression_stmt>(prev(), std::move(expr));
}

ast::stmt_ptr parser::while_statement() {
    consume(tt::LEFT_PAREN, err::expected_left_paren_while);
    auto condition = expression();
    consume(tt::RIGHT_PAREN, err::expected_right_paren_condition);

    auto body = statement();
    return make_stmt<ast::while_stmt>(prev(), std::move(condition), std::move(body));
}

ast::stmt_ptr parser::for_statement() {
    consume(tt::LEFT_PAREN, err::expected_left_paren_for);

    ast::stmt_ptr initializer;
    if (match({ tt::SEMICOLON })) {}
    else if (match({ tt::KEYWORD })) {
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
    return make_stmt<ast::for_stmt>(prev(),
        std::move(initializer), std::move(condition),
        std::move(increment), std::move(body));
}

ast::stmt_ptr parser::if_statement() {
    consume(tt::LEFT_PAREN, err::expected_left_paren_if);
    auto condition = expression();
    consume(tt::RIGHT_PAREN, err::expected_right_paren_condition);

    auto then_branch = statement();
    ast::stmt_ptr else_branch;

    if (match({ tt::KEYWORD })) {
        auto kw = prev().as_keyword();
        if (kw && kw->lexeme_ == "else") {
            else_branch = statement();
        } else {
            current_--;
        }
    }

    return make_stmt<ast::if_stmt>(prev(), std::move(condition), 
        std::move(then_branch), std::move(else_branch));
}

ast::stmt_ptr parser::return_statement() {
    const auto& keyword = prev();

    std::optional<ast::expression> value;
    if (!check(tt::SEMICOLON)) value = expression();
    consume(tt::SEMICOLON, err::expected_semicolon);

    return make_stmt<ast::return_stmt>(keyword, keyword, std::move(value));
}

ast::stmt_ptr parser::block_statement() {
    ast::block_stmt block;
    while (!check(tt::RIGHT_BRACE) && !is_at_end()) {
        auto stmt = declaration();
        if (stmt) block.statements_.push_back(std::move(stmt));
    }
    consume(tt::RIGHT_BRACE, err::expected_right_brace);
    const auto& brace = prev();
    return make_stmt<ast::block_stmt>(brace, std::move(block.statements_));
}

ast::expression parser::expression() { return assignment(); }

ast::expression parser::assignment() {
    auto expr = logic_or();

    if (match({ tt::EQUAL, tt::PLUS_EQUAL,
                tt::MINUS_EQUAL, tt::STAR_EQUAL,
                tt::SLASH_EQUAL, tt::PERCENT_EQUAL })) {
        const auto& op = prev();
        auto value = assignment();
        return make_expr<ast::binary_expr>(op, std::move(expr), op, std::move(value));
    }
    return expr;
}

ast::expression parser::logic_or() {
    return parse_binary({ tt::OR }, [this] { return logic_and(); });
}

ast::expression parser::logic_and() {
    return parse_binary({ tt::AND }, [this] { return equality(); });
}

ast::expression parser::equality() {
    return parse_binary({ tt::EQUAL_EQUAL, tt::BANG_EQUAL },
        [this] { return comparison(); });
}

ast::expression parser::comparison() {
    return parse_binary({ tt::GREATER, tt::GREATER_EQUAL, tt::LESS, tt::LESS_EQUAL },
        [this] { return term(); });
}

ast::expression parser::term() {
    return parse_binary({ tt::PLUS, tt::MINUS },
        [this] { return factor(); });
}

ast::expression parser::factor() {
    return parse_binary({ tt::STAR, tt::SLASH, tt::PERCENT },
        [this] { return unary(); });
}

ast::expression parser::parse_binary(
    std::initializer_list<tt> operators,
    std::function<ast::expression()> sub_parser) {

    auto left = sub_parser();
    while (match(operators)) {
        const auto& op = prev();
        auto right = sub_parser();
        left = make_expr<ast::binary_expr>(op, std::move(left), op, std::move(right));
    }
    return left;
}

ast::expression parser::unary() {
    if (match({ tt::BANG, tt::MINUS, tt::INCREMENT, tt::DECREMENT })) {
        const auto& op = prev();
        auto operand = unary();
        return make_expr<ast::unary_expr>(op, op, std::move(operand));
    }
    return postfix();
}

ast::expression parser::postfix() {
    auto expr = primary();

    while (true) {
        if (match({ tt::LEFT_BRACKET })) {
			expr = finish_index(std::move(expr));
		} else if (match({ tt::INCREMENT, tt::DECREMENT })) {
			const auto& op = prev();
			expr = make_expr<ast::postfix_expr>(op, std::move(expr), op);
        } else {
            break;
        }
        
    }

    return expr;
}

ast::expression parser::array_literal() {
	std::vector<ast::expression> elements;
	const auto& brace = prev();
	if (!check(tt::RIGHT_BRACE)) {
		do {
			elements.push_back(expression());
		} while (match({ tt::COMMA }));
	}
	consume(tt::RIGHT_BRACE, err::expected_right_brace);
	return make_expr<ast::array_literal_expr>(brace, std::move(elements));
}

ast::expression parser::primary() {
    if (match({ tt::NUMBER, tt::STRING })) {
        return make_expr_val<ast::literal_expr>(prev());
    }

    if (match({ tt::KEYWORD })) {
        auto kw = prev().as_keyword();
        if (kw && (kw->lexeme_ == "true" || kw->lexeme_ == "false")) {
            return make_expr_val<ast::literal_expr>( prev());
        }
        reporter_.parse_error(prev(), err::expected_expression);
    }

    if (match({ tt::IDENTIFIER })) {
        const auto& name = prev();

        if (match({ tt::LEFT_PAREN })) return finish_call(name);
        return make_expr_val<ast::variable_expr>(name);
    }

    if (match({ tt::LEFT_PAREN })) {
        auto expr = expression();
        consume(tt::RIGHT_PAREN, err::expected_right_paren);
        return expr;
    }

	if (match({ tt::LEFT_BRACE })) return array_literal();
		
    reporter_.parse_error(peek(), err::expected_expression);
}

ast::expression parser::finish_call(const core::token& callee) {
    std::vector<ast::expression> args;

    if (!check(tt::RIGHT_PAREN)) {
        do {
            args.push_back(expression());
        } while (match({ tt::COMMA }));
    }

    consume(tt::RIGHT_PAREN, err::expected_right_paren);

    return make_expr<ast::call_expr>(callee, callee, std::move(args));
}

ast::expression parser::finish_index(ast::expression object) {
	const auto& bracket = prev();
	auto index = expression();
	consume(tt::RIGHT_BRACKET, err::expected_right_bracket);
	return make_expr<ast::index_expr>(bracket, std::move(object), std::move(index));
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

} // namespace parser