// parser/parser.cpp

#include "parser/parser.hpp"

#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/error/diagnostic.hpp"
#include "core/error/error_codes.hpp"
#include "core/token/token_types.hpp"
#include "core/value/conversions.hpp"
#include "core/value/value.hpp"
#include "parser/slot_allocator.hpp"

#include <algorithm>
#include <array>
#include <span>
#include <utility>
#include <vector>

namespace cint {

using tt = core::token_type;
using err = core::error_code;

namespace {

struct infix_rule {
    tt type_;
    std::int8_t precedence_;
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

bool is_statement_start(tt type) noexcept {
    return type == tt::KW_LET || type == tt::KW_FN || type == tt::KW_IF || type == tt::KW_WHILE || type == tt::KW_FOR ||
           type == tt::KW_RETURN || type == tt::IDENTIFIER;
}

struct parse_error : std::exception {
    const char* what() const noexcept override { return "Syntax error"; }
};

template <typename... Args>
[[noreturn]] void raise_error(core::diagnostic_sink& sink, core::location loc, core::error_code code, Args&&... args) {
    sink.report(loc, code, std::forward<Args>(args)...);
    throw parse_error{};
}

constexpr std::uint32_t THIS_SLOT = std::numeric_limits<std::uint32_t>::max();

class slot_scope_guard {
public:
    slot_scope_guard(slot_allocator& s) : s_{s} { s_.push_scope(); }
    ~slot_scope_guard() { s_.pop_scope(); }
    slot_scope_guard(const slot_scope_guard&) = delete;
    slot_scope_guard& operator=(const slot_scope_guard&) = delete;

private:
    slot_allocator& s_;
};

class slot_frame_guard {
public:
    slot_frame_guard(slot_allocator& s) : s_{s} { s_.begin_frame(); }

    std::uint32_t end() {
        ended_ = true;
        return s_.end_frame();
    }

    ~slot_frame_guard() {
        if (!ended_) std::ignore = s_.end_frame();
    }

    slot_frame_guard(const slot_frame_guard&) = delete;
    slot_frame_guard& operator=(const slot_frame_guard&) = delete;

private:
    slot_allocator& s_;
    bool ended_{};
};

}  // namespace

ast::stmt_list parser::parse() {
    auto frame = slot_frame_guard{slots_};

    auto statements = ast::stmt_list{arena_};
    while (!is_at_end()) {
        auto&& stmt = declaration();
        if (stmt) statements.push_back(std::move(*stmt));
    }

    global_max_slots_ = frame.end();
    return statements;
}

bool parser::match(std::initializer_list<tt> types) noexcept {
    const auto matched = std::ranges::any_of(types, [&](auto&& type) { return check(type); });
    if (matched) advance();
    return matched;
}

const core::token& parser::consume(tt type, err code) {
    if (check(type)) return advance();
    if (type == tt::SEMICOLON && (is_statement_start(peek().type_) || check(tt::RIGHT_BRACE))) {
        sink_.report(peek().loc_, code);
        return prev();
    }
    raise_error(sink_, peek().loc_, code);
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
    try {
        if (match({tt::KW_FN})) return fn_declaration();
        if (match({tt::KW_STRUCT})) return struct_declaration();
        if (match({tt::KW_LET})) return let_declaration();
        return statement();
    } catch (const parse_error&) {
        synchronize();
        return std::nullopt;
    }
}

ast::statement parser::let_declaration() {
    const auto name = consume(tt::IDENTIFIER, err::expected_identifier);
    const auto slot = slots_.declare(name.lexeme_);
    consume(tt::EQUAL, err::expected_equal);
    auto&& init = assignment();
    consume(tt::SEMICOLON, err::expected_semicolon);
    return ast::make_stmt<ast::var_declaration_stmt>(arena_, name, slot, std::move(init));
}

ast::statement parser::fn_declaration() {
    const auto name = consume(tt::IDENTIFIER, err::expected_identifier);
    consume(tt::LEFT_PAREN, err::expected_left_paren);

    auto frame = slot_frame_guard{slots_};

    auto params = core::arena_vector<core::token>{arena_};
    params.reserve(4);
    if (!check(tt::RIGHT_PAREN)) {
        do {
            const auto p = consume(tt::IDENTIFIER, err::expected_identifier);
            std::ignore = slots_.declare(p.lexeme_);
            params.push_back(p);
        } while (match({tt::COMMA}) && !check(tt::RIGHT_PAREN));
    }
    consume(tt::RIGHT_PAREN, err::expected_right_paren);
    consume(tt::LEFT_BRACE, err::expected_left_brace);

    auto&& block_stmt = block_statement();
    auto&& body = std::get<core::arena_ptr<ast::block_stmt>>(block_stmt)->statements_;

    return ast::make_stmt<ast::func_declaration_stmt>(arena_, name, std::move(params), std::move(body), frame.end(),
                                                      name.loc_);
}

ast::statement parser::struct_declaration() {
    const auto name = consume(tt::IDENTIFIER, err::expected_identifier);
    consume(tt::LEFT_BRACE, err::expected_left_brace);

    auto field_names = std::vector<std::string_view>{};
    auto methods = core::arena_vector<ast::func_declaration_stmt>{arena_};

    while (!check(tt::RIGHT_BRACE) && !is_at_end()) {
        if (match({tt::KW_FN})) {
            auto&& fn = fn_declaration();
            methods.push_back(std::move(*std::get<core::arena_ptr<ast::func_declaration_stmt>>(fn)));
        } else {
            auto&& fname = consume(tt::IDENTIFIER, err::expected_identifier);
            field_names.push_back(fname.lexeme_);
            match({tt::COMMA, tt::SEMICOLON});
        }
    }
    consume(tt::RIGHT_BRACE, err::expected_right_brace);

    auto&& st = core::struct_type{name.lexeme_, std::move(field_names)};
    return ast::make_stmt<ast::struct_declaration_stmt>(arena_, std::move(st), std::move(methods), name.loc_);
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
    const auto start_loc = prev().loc_;
    auto&& condition = assignment();
    auto&& body = statement();
    return ast::make_stmt<ast::while_stmt>(arena_, std::move(condition), std::move(body), start_loc);
}

ast::statement parser::for_statement() {
    const auto start_loc = prev().loc_;
    consume(tt::LEFT_PAREN, err::expected_left_paren_for);

    auto guard = slot_scope_guard{slots_};

    auto initializer = std::optional<ast::statement>{};
    if (match({tt::SEMICOLON})) {
    } else if (match({tt::KW_LET})) {
        initializer = let_declaration();
    } else {
        initializer = statement();
    }

    auto condition = std::optional<ast::expression>{};
    if (!check(tt::SEMICOLON)) condition = assignment();
    consume(tt::SEMICOLON, err::expected_semicolon);

    auto increment = std::optional<ast::expression>{};
    if (!check(tt::RIGHT_PAREN)) increment = assignment();
    consume(tt::RIGHT_PAREN, err::expected_right_paren);

    auto&& body = statement();

    return ast::make_stmt<ast::for_stmt>(arena_, std::move(initializer), std::move(condition), std::move(increment),
                                         std::move(body), start_loc);
}

ast::statement parser::if_statement() {
    const auto start_loc = prev().loc_;
    auto&& condition = assignment();
    auto&& then_branch = statement();
    auto else_branch = std::optional<ast::statement>{};

    if (match({tt::KW_ELSE})) else_branch = statement();

    return ast::make_stmt<ast::if_stmt>(arena_, std::move(condition), std::move(then_branch), std::move(else_branch),
                                        start_loc);
}

ast::statement parser::return_statement() {
    const auto start_loc = prev().loc_;
    auto value = std::optional<ast::expression>{};
    if (!check(tt::SEMICOLON)) value = assignment();
    consume(tt::SEMICOLON, err::expected_semicolon);

    return ast::make_stmt<ast::return_stmt>(arena_, std::move(value), start_loc);
}

ast::statement parser::block_statement() {
    const auto start_loc = prev().loc_;
    auto guard = slot_scope_guard{slots_};

    auto statements = ast::stmt_list{arena_};
    while (!check(tt::RIGHT_BRACE) && !is_at_end()) {
        auto&& stmt = [&] {
            if (match({tt::KW_LET})) return let_declaration();
            if (check(tt::KW_FN) || check(tt::KW_STRUCT)) raise_error(sink_, peek().loc_, err::nested_declaration);
            return statement();
        }();
        statements.push_back(std::move(stmt));
    }
    consume(tt::RIGHT_BRACE, err::expected_right_brace);

    return ast::make_stmt<ast::block_stmt>(arena_, std::move(statements), start_loc);
}

ast::expression parser::assignment() {
    auto&& left = parse_expression(0);

    if (match({tt::EQUAL, tt::PLUS_EQUAL, tt::MINUS_EQUAL, tt::STAR_EQUAL, tt::SLASH_EQUAL, tt::PERCENT_EQUAL,
               tt::BIT_AND_EQUAL, tt::BIT_OR_EQUAL, tt::XOR_EQUAL, tt::SHL_EQUAL, tt::SHR_EQUAL})) {
        const auto op = prev();
        auto&& right = assignment();
        return ast::make_expr<ast::assignment_expr>(arena_, std::move(left), op, std::move(right));
    }

    return left;
}

ast::expression parser::parse_expression(std::int8_t precedence) {
    auto&& left = unary();

    while (true) {
        const auto op_type = peek().type_;
        const auto it = std::ranges::find(infix_table, op_type, &infix_rule::type_);
        if (it == infix_table.end() || it->precedence_ < precedence) break;

        const auto op = advance();
        auto&& right = parse_expression(it->precedence_ + 1);
        left = ast::make_expr<ast::binary_expr>(arena_, std::move(left), op, std::move(right));
    }

    return left;
}

ast::expression parser::unary() {
    if (match({tt::BANG, tt::MINUS, tt::INCREMENT, tt::DECREMENT, tt::BIT_NOT})) {
        const auto op = prev();
        auto&& operand = unary();
        return ast::make_expr<ast::unary_expr>(arena_, op, std::move(operand));
    }
    return postfix();
}

ast::expr_list parser::parse_call_args() {
    auto args = ast::expr_list{arena_};
    if (!check(tt::RIGHT_PAREN)) {
        do { args.push_back(assignment()); } while (match({tt::COMMA}));
    }
    consume(tt::RIGHT_PAREN, err::expected_right_paren);
    return args;
}

ast::expression parser::postfix() {
    auto&& expr = primary();

    while (true) {
        if (match({tt::DOT})) {
            auto&& member = consume(tt::IDENTIFIER, err::expected_identifier);
            expr = match({tt::LEFT_PAREN})
                       ? ast::make_expr<ast::method_call_expr>(arena_, std::move(expr), member, parse_call_args())
                       : ast::make_expr<ast::member_access_expr>(arena_, std::move(expr), member);
        } else if (match({tt::INCREMENT, tt::DECREMENT})) {
            expr = ast::make_expr<ast::postfix_expr>(arena_, std::move(expr), prev());
        } else {
            break;
        }
    }
    return expr;
}

ast::expression parser::primary() {
    if (match({tt::NUMBER})) {
        const auto [_, lex, loc] = prev();
        const auto is_double = lex.find('.') != std::string_view::npos;
        auto from_string = [&]<core::numerical T> {
            const auto num = core::to_numbers<T>(lex);
            if (!num) raise_error(sink_, loc, err::invalid_number);
            return ast::make_expr<ast::literal_expr>(arena_, *num, loc);
        };
        return is_double ? from_string.operator()<core::value::double_t>()
                         : from_string.operator()<core::value::int_t>();
    }

    if (match({tt::STRING})) {
        const auto token = prev();
        const auto raw = token.lexeme_.substr(1, token.lexeme_.size() - 2);
        return ast::make_expr<ast::literal_expr>(arena_, core::value::string_t{raw}, token.loc_);
    }

    if (match({tt::KW_TRUE})) return ast::make_expr<ast::literal_expr>(arena_, true, prev().loc_);
    if (match({tt::KW_FALSE})) return ast::make_expr<ast::literal_expr>(arena_, false, prev().loc_);

    if (match({tt::IDENTIFIER})) {
        const auto name = prev();
        if (match({tt::LEFT_PAREN})) return ast::make_expr<ast::call_expr>(arena_, name, parse_call_args());

        if (name.lexeme_ == "this") return ast::make_expr<ast::variable_expr>(arena_, name, THIS_SLOT);

        const auto slot = slots_.resolve(name.lexeme_);
        if (!slot) raise_error(sink_, name.loc_, err::undefined_variable, name.lexeme_);
        return ast::make_expr<ast::variable_expr>(arena_, name, *slot);
    }

    if (match({tt::LEFT_PAREN})) {
        auto&& expr = assignment();
        consume(tt::RIGHT_PAREN, err::expected_right_paren);
        return expr;
    }

    raise_error(sink_, peek().loc_, err::expected_expression);
}

void parser::synchronize() {
    if (is_statement_start(peek().type_) && (prev().type_ == tt::SEMICOLON || prev().type_ == tt::RIGHT_BRACE)) return;

    advance();

    auto brace_depth = std::uint32_t{};
    while (!is_at_end()) {
        const auto type = peek().type_;

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
            return;
        }
        if (brace_depth == 0) {
            if (type == tt::SEMICOLON) {
                advance();
                return;
            }
            if (is_statement_start(type) && (prev().type_ == tt::SEMICOLON || prev().type_ == tt::RIGHT_BRACE)) return;
        }
        advance();
    }
}
}  // namespace cint
