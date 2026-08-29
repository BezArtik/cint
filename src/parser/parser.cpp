// parser/parser.cpp

#include "parser/parser.hpp"

#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/error/error_codes.hpp"
#include "core/token/keywords.hpp"
#include "core/token/token_types.hpp"

#include <algorithm>
#include <array>
#include <memory_resource>
#include <span>
#include <utility>
#include <vector>

using tt = core::token_type;
using err = core::error_code;

namespace {

struct infix_rule {
    tt type_;
    int8_t precedence_;
};

// clang-format off
constexpr std::array infix_table = {
    infix_rule{tt::LOGICAL_OR, 2},  
    infix_rule{tt::LOGICAL_AND, 3},

    infix_rule{tt::BIT_OR, 4},      
    infix_rule{tt::XOR, 5},
    infix_rule{tt::BIT_AND, 6},

    infix_rule{tt::EQUAL_EQUAL, 7}, 
    infix_rule{tt::BANG_EQUAL, 7},
    infix_rule{tt::LESS, 8},        
    infix_rule{tt::LESS_EQUAL, 8},
    infix_rule{tt::GREATER, 8},     
    infix_rule{tt::GREATER_EQUAL, 8},

    infix_rule{tt::SHL, 9},        
    infix_rule{tt::SHR, 9},

    infix_rule{tt::PLUS, 10},       
    infix_rule{tt::MINUS, 10},

    infix_rule{tt::STAR, 11},      
    infix_rule{tt::SLASH, 11},
    infix_rule{tt::PERCENT, 11},
};
// clang-format on

bool is_type_start(tt type) noexcept {
    return type == tt::KW_INT || type == tt::KW_DOUBLE || type == tt::KW_BOOL || type == tt::KW_STRING ||
           type == tt::KW_VOID || type == tt::KW_STRUCT;
}

}  // namespace

ast::stmt_list parser::parse() {
    ast::stmt_list statements{&mr_};
    while (!is_at_end()) {
        auto&& stmt = declaration();
        if (stmt) statements.push_back(std::move(*stmt));
    }
    return statements;
}

bool parser::match(std::initializer_list<tt> types) noexcept {
    auto&& matched = std::ranges::any_of(types, [&](auto&& type) { return check(type); });
    if (matched) advance();
    return matched;
}

const core::token& parser::consume(tt type, err code) {
    if (check(type)) return advance();
    reporter_.parse_error(peek().loc_, code);
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
    return tokens_[current_ - 1];
}

std::optional<ast::statement> parser::declaration() {
    auto&& parse_decl = [&](auto&& type) {
        auto&& name = consume(tt::IDENTIFIER, err::expected_identifier);
        return match({tt::LEFT_PAREN}) ? func_declaration(std::move(type), name)
                                       : var_declaration(std::move(type), name);
    };
    try {
        if (match({tt::KW_STRUCT})) {
            auto&& name = consume(tt::IDENTIFIER, err::expected_identifier);
            return check(tt::LEFT_BRACE) ? struct_declaration(name)
                                         : parse_decl(core::type::struct_type(name.lexeme_, {}));
        }
        if (is_type_start(peek().type_)) return parse_decl(parse_type());
        return statement();
    } catch (const core::parse_error&) {
        synchronize();
        return std::nullopt;
    }
}

core::type parser::parse_array_dimensions(core::type base_type) {
    std::array<std::byte, 256> buffer;
    std::pmr::monotonic_buffer_resource local_mr{buffer.data(), buffer.size()};
    std::pmr::vector<size_t> dimensions{&local_mr};

    while (match({tt::LEFT_BRACKET})) {
        size_t dim_size = 0;
        if (match({tt::NUMBER})) {
            auto&& size_token = prev();
            try {
                auto&& val = core::value::from_string(size_token.lexeme_, false);
                dim_size = val.to_int();
            } catch (const core::value_error&) { reporter_.parse_error(size_token.loc_, err::unexpected_token); }
            if (dim_size == 0) reporter_.parse_error(size_token.loc_, err::unexpected_token);
        }
        consume(tt::RIGHT_BRACKET, err::expected_right_bracket);
        dimensions.push_back(dim_size);
    }

    for (auto it = dimensions.rbegin(), endit = dimensions.rend(); it != endit; ++it)
        base_type = core::type::array_type(base_type, *it);

    return base_type;
}

ast::statement parser::var_declaration(core::type type, const core::token& name) {
    if (check(tt::LEFT_BRACKET)) type = parse_array_dimensions(type);

    std::optional<ast::expression> initializer;
    if (match({tt::EQUAL})) initializer = match({tt::LEFT_BRACE}) ? initializer_list() : assignment();
    consume(tt::SEMICOLON, err::expected_semicolon);
    return ast::make_stmt<ast::var_declaration_stmt>(arena_, std::move(type), name, std::move(initializer));
}

ast::statement parser::func_declaration(const core::type& return_type, const core::token& name) {
    std::vector<core::type::param_t> params;
    params.reserve(4);
    if (!check(tt::RIGHT_PAREN)) {
        do {
            auto&& type = parse_type();
            auto&& name = consume(tt::IDENTIFIER, err::expected_identifier);
            if (check(tt::LEFT_BRACKET)) type = parse_array_dimensions(type);
            params.emplace_back(name.lexeme_, std::move(type));
        } while (match({tt::COMMA}));
    }

    consume(tt::RIGHT_PAREN, err::expected_right_paren);
    consume(tt::LEFT_BRACE, err::expected_left_brace);

    auto&& body = block_statement();
    auto&& func_type = core::type::function_type(name.lexeme_, return_type, std::move(params));

    return ast::make_stmt<ast::func_declaration_stmt>(arena_, std::move(func_type), std::move(body), name.loc_);
}

ast::statement parser::struct_declaration(const core::token& name) {
    consume(tt::LEFT_BRACE, err::expected_left_brace);

    std::vector<core::type::field_t> fields;
    fields.reserve(4);

    while (!check(tt::RIGHT_BRACE) && !is_at_end()) {
        auto&& field_type = parse_type();
        auto&& field_name = consume(tt::IDENTIFIER, err::expected_identifier);
        if (check(tt::LEFT_BRACKET)) field_type = parse_array_dimensions(field_type);
        consume(tt::SEMICOLON, err::expected_semicolon);
        fields.emplace_back(field_name.lexeme_, std::move(field_type));
    }

    consume(tt::RIGHT_BRACE, err::expected_right_brace);
    consume(tt::SEMICOLON, err::expected_semicolon);

    auto&& struct_type = core::type::struct_type(name.lexeme_, std::move(fields));

    return ast::make_stmt<ast::struct_declaration_stmt>(arena_, std::move(struct_type), name.loc_);
}

core::type parser::parse_type() {
    if (match({tt::KW_STRUCT})) {
        auto&& name = consume(tt::IDENTIFIER, err::expected_identifier);
        return core::type::struct_type(name.lexeme_, {});
    }

    if (match({tt::KW_INT, tt::KW_DOUBLE, tt::KW_BOOL, tt::KW_STRING, tt::KW_VOID})) {
        auto&& kw = prev();

        auto&& it = std::ranges::find(core::keywords, kw.type_, &core::keyword_info::type_);
        if (it == core::keywords.end()) reporter_.parse_error(kw.loc_, err::expected_type);
        return it->semantic_type_;
    }

    reporter_.parse_error(peek().loc_, err::expected_type);
}

ast::statement parser::statement() {
    if (match({tt::KW_WHILE})) return while_statement();
    if (match({tt::KW_FOR})) return for_statement();
    if (match({tt::KW_IF})) return if_statement();
    if (match({tt::KW_RETURN})) return return_statement();
    if (match({tt::LEFT_BRACE})) return block_statement();

    auto&& expr = assignment();
    consume(tt::SEMICOLON, err::expected_semicolon);

    return ast::make_stmt<ast::expression_stmt>(arena_, std::move(expr));
}

ast::statement parser::while_statement() {
    consume(tt::LEFT_PAREN, err::expected_left_paren_while);
    auto&& condition = assignment();
    consume(tt::RIGHT_PAREN, err::expected_right_paren_condition);

    auto&& body = statement();
    return ast::make_stmt<ast::while_stmt>(arena_, std::move(condition), std::move(body), prev().loc_);
}

ast::statement parser::for_statement() {
    consume(tt::LEFT_PAREN, err::expected_left_paren_for);

    std::optional<ast::statement> initializer;
    if (match({tt::SEMICOLON})) {
    } else if (is_type_start(peek().type_)) {
        auto&& type = parse_type();
        initializer = var_declaration(type, consume(tt::IDENTIFIER, err::expected_identifier));
    } else {
        initializer = statement();
    }

    std::optional<ast::expression> condition;
    if (!check(tt::SEMICOLON)) condition = assignment();
    consume(tt::SEMICOLON, err::expected_semicolon);

    std::optional<ast::expression> increment;
    if (!check(tt::RIGHT_PAREN)) increment = assignment();
    consume(tt::RIGHT_PAREN, err::expected_right_paren);

    auto&& body = statement();
    return ast::make_stmt<ast::for_stmt>(arena_, std::move(initializer), std::move(condition), std::move(increment),
                                         std::move(body), prev().loc_);
}

ast::statement parser::if_statement() {
    consume(tt::LEFT_PAREN, err::expected_left_paren_if);
    auto&& condition = assignment();
    consume(tt::RIGHT_PAREN, err::expected_right_paren_condition);

    auto&& then_branch = statement();
    std::optional<ast::statement> else_branch;

    if (match({tt::KW_ELSE})) else_branch = statement();

    return ast::make_stmt<ast::if_stmt>(arena_, std::move(condition), std::move(then_branch), std::move(else_branch),
                                        prev().loc_);
}

ast::statement parser::return_statement() {
    std::optional<ast::expression> value;
    if (!check(tt::SEMICOLON)) value = assignment();
    consume(tt::SEMICOLON, err::expected_semicolon);

    return ast::make_stmt<ast::return_stmt>(arena_, std::move(value), prev().loc_);
}

ast::statement parser::block_statement() {
    ast::stmt_list statements{&mr_};
    auto&& has_decls = false;
    while (!check(tt::RIGHT_BRACE) && !is_at_end()) {
        auto&& stmt = declaration();
        if (stmt) {
            if (stmt->holds<ast::var_declaration_stmt>()) has_decls = true;
            statements.push_back(std::move(*stmt));
        }
    }
    consume(tt::RIGHT_BRACE, err::expected_right_brace);
    return ast::make_stmt<ast::block_stmt>(arena_, std::move(statements), has_decls, prev().loc_);
}

ast::expression parser::assignment() {
    auto&& left = parse_expression(0);

    if (match({tt::EQUAL, tt::PLUS_EQUAL, tt::MINUS_EQUAL, tt::STAR_EQUAL, tt::SLASH_EQUAL, tt::PERCENT_EQUAL,
               tt::BIT_AND_EQUAL, tt::BIT_OR_EQUAL, tt::BIT_NOT_EQUAL, tt::XOR_EQUAL, tt::SHL_EQUAL, tt::SHR_EQUAL})) {
        auto&& op = prev();
        auto&& right = assignment();
        return ast::make_expr<ast::assignment_expr>(arena_, std::move(left), op, std::move(right));
    }

    return left;
}

ast::expression parser::parse_expression(int8_t precedence) {
    auto&& left = unary();

    while (true) {
        auto&& op_type = peek().type_;
        auto&& it = std::ranges::find(infix_table, op_type, &infix_rule::type_);
        if (it == infix_table.end() || it->precedence_ < precedence) break;

        auto&& op = advance();
        auto&& right = parse_expression(it->precedence_ + 1);
        left = ast::make_expr<ast::binary_expr>(arena_, std::move(left), op, std::move(right));
    }

    return left;
}

ast::expression parser::unary() {
    if (match({tt::BANG, tt::MINUS, tt::INCREMENT, tt::DECREMENT, tt::BIT_NOT})) {
        auto&& op = prev();
        auto&& operand = unary();
        return ast::make_expr<ast::unary_expr>(arena_, op, std::move(operand));
    }
    return postfix();
}

ast::expression parser::postfix() {
    auto&& expr = primary();

    while (true) {
        if (match({tt::LEFT_BRACKET})) {
            expr = finish_index(std::move(expr));
        } else if (match({tt::DOT})) {
            auto&& member = consume(tt::IDENTIFIER, err::expected_identifier);
            expr = ast::make_expr<ast::member_access_expr>(arena_, std::move(expr), member);
        } else if (match({tt::INCREMENT, tt::DECREMENT})) {
            expr = ast::make_expr<ast::postfix_expr>(arena_, std::move(expr), prev());
        } else {
            break;
        }
    }

    return expr;
}

ast::expression parser::initializer_list() {
    ast::expr_list elements{&mr_};

    if (!check(tt::RIGHT_BRACE)) {
        do {
            elements.push_back(match({tt::LEFT_BRACE}) ? initializer_list() : assignment());
        } while (match({tt::COMMA}));
    }
    consume(tt::RIGHT_BRACE, err::expected_right_brace);
    return ast::make_expr<ast::initializer_list_expr>(arena_, std::move(elements), prev().loc_);
}
// clang-format off
std::string parser::process_escape_sequences(std::string_view raw, core::location start_loc) {
    std::string result;
    result.reserve(raw.size());

    auto&& it = raw.begin();
    auto&& end = raw.end();

    while (it != end) {
        if (*it == '\\') {
            auto&& next = std::next(it);
            if (next == end) {
                reporter_.error(start_loc, err::unterminated_string);
                break;
            }
            switch (*next) {
                case 'n':  result.push_back('\n'); break;
                case 't':  result.push_back('\t'); break;
                case 'r':  result.push_back('\r'); break;
                case '\\': result.push_back('\\'); break;
                case '"':  result.push_back('"');  break;
                case '0':  result.push_back('\0'); break;
                default:
                    reporter_.error(start_loc, err::unexpected_character, *next);
                    result.push_back(*next);
                    break;
            }
            std::advance(it, 2);
        } else {
            result.push_back(*it);
            ++it;
        }
    }

    return result;
}
// clang-format on
ast::expression parser::primary() {
    if (match({tt::NUMBER})) {
        auto&& token = prev();
        auto&& is_double = token.lexeme_.find('.') != std::string_view::npos;
        auto&& val = core::value::from_string(token.lexeme_, is_double);
        return ast::make_expr<ast::literal_expr>(arena_, std::move(val), token.loc_);
    }

    if (match({tt::STRING})) {
        auto&& token = prev();
        auto&& raw = token.lexeme_.substr(1, token.lexeme_.size() - 2);
        auto&& processed = process_escape_sequences(raw, token.loc_);
        return ast::make_expr<ast::literal_expr>(arena_, std::move(processed), token.loc_);
    }

    if (match({tt::KW_TRUE})) return ast::make_expr<ast::literal_expr>(arena_, true, prev().loc_);
    if (match({tt::KW_FALSE})) return ast::make_expr<ast::literal_expr>(arena_, false, prev().loc_);

    if (match({tt::IDENTIFIER})) {
        auto&& name = prev();
        return match({tt::LEFT_PAREN}) ? finish_call(name) : ast::make_expr<ast::variable_expr>(arena_, name);
    }

    if (match({tt::LEFT_PAREN})) {
        auto&& expr = assignment();
        consume(tt::RIGHT_PAREN, err::expected_right_paren);
        return expr;
    }

    if (match({tt::LEFT_BRACE})) return initializer_list();

    reporter_.parse_error(peek().loc_, err::expected_expression);
}

ast::expression parser::finish_call(const core::token& callee) {
    ast::expr_list args{&mr_};

    if (!check(tt::RIGHT_PAREN)) {
        do { args.push_back(assignment()); } while (match({tt::COMMA}));
    }

    consume(tt::RIGHT_PAREN, err::expected_right_paren);

    return ast::make_expr<ast::call_expr>(arena_, callee, std::move(args));
}

ast::expression parser::finish_index(ast::expression object) {
    auto&& index = assignment();
    consume(tt::RIGHT_BRACKET, err::expected_right_bracket);
    return ast::make_expr<ast::index_expr>(arena_, std::move(object), std::move(index), prev().loc_);
}

void parser::synchronize() {
    advance();

    uint32_t brace_depth = 0;

    while (!is_at_end()) {
        auto&& type = peek().type_;

        if (type == tt::LEFT_BRACE) {
            ++brace_depth;
            advance();
            continue;
        }

        if (type == tt::RIGHT_BRACE) {
            if (brace_depth > 0) {
                --brace_depth;
                advance();
                continue;
            }
            advance();
            return;
        }

        if (brace_depth == 0) {
            if (prev().type_ == tt::SEMICOLON || type == tt::LEFT_BRACE) return;
            auto&& it = std::ranges::find(core::keywords, type, &core::keyword_info::type_);
            if (it != core::keywords.end() && it->can_start_statement_ && prev().type_ == tt::RIGHT_BRACE) return;
        }

        advance();
    }
}
