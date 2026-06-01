// debug/debug.cpp


#include "debug/debug.hpp"
#include "core/token/token_types.hpp"
#include "core/token/keywords.hpp"
#include "core/utils/overloaded.hpp"
#include "runtime/value.hpp"
#include <iostream>
#include <iomanip>
#include <string>
#include <cstdint>


namespace debug {

namespace {

std::string indent_str(uint32_t level) {
    return std::string(static_cast<size_t>(level) * 2, ' ');
}

const char* token_type_name(core::token_type t) {
    switch (t) {
    case core::token_type::LEFT_PAREN:    return "LEFT_PAREN";
    case core::token_type::RIGHT_PAREN:   return "RIGHT_PAREN";
    case core::token_type::LEFT_BRACE:    return "LEFT_BRACE";
    case core::token_type::RIGHT_BRACE:   return "RIGHT_BRACE";
	case core::token_type::LEFT_BRACKET:  return "LEFT_BRACKET";
	case core::token_type::RIGHT_BRACKET: return "RIGHT_BRACKET";
    case core::token_type::COMMA:         return "COMMA";
    case core::token_type::DOT:           return "DOT";
    case core::token_type::SEMICOLON:     return "SEMICOLON";
    case core::token_type::PLUS:          return "PLUS";
    case core::token_type::MINUS:         return "MINUS";
    case core::token_type::STAR:          return "STAR";
    case core::token_type::SLASH:         return "SLASH";
    case core::token_type::PERCENT:       return "PERCENT";
    case core::token_type::BANG:          return "BANG";
    case core::token_type::EQUAL:         return "EQUAL";
    case core::token_type::BANG_EQUAL:    return "BANG_EQUAL";
    case core::token_type::EQUAL_EQUAL:   return "EQUAL_EQUAL";
    case core::token_type::GREATER:       return "GREATER";
    case core::token_type::GREATER_EQUAL: return "GREATER_EQUAL";
    case core::token_type::LESS:          return "LESS";
    case core::token_type::LESS_EQUAL:    return "LESS_EQUAL";
    case core::token_type::INCREMENT:     return "INCREMENT";
    case core::token_type::DECREMENT:     return "DECREMENT";
    case core::token_type::PLUS_EQUAL:    return "PLUS_EQUAL";
    case core::token_type::MINUS_EQUAL:   return "MINUS_EQUAL";
    case core::token_type::STAR_EQUAL:    return "STAR_EQUAL";
    case core::token_type::SLASH_EQUAL:   return "SLASH_EQUAL";
    case core::token_type::PERCENT_EQUAL: return "PERCENT_EQUAL";
    case core::token_type::AND:           return "AND";
    case core::token_type::OR:            return "OR";
    case core::token_type::IDENTIFIER:    return "IDENTIFIER";
    case core::token_type::STRING:        return "STRING";
    case core::token_type::NUMBER:        return "NUMBER";
    case core::token_type::KEYWORD:       return "KEYWORD";
    case core::token_type::END_OF_FILE:   return "EOF";
    case core::token_type::UNKNOWN:       return "UNKNOWN";
    default:                              return "???";
    }
}



void print_literal(const ast::literal_expr& e, uint32_t level) {
    std::cerr << indent_str(level)
        << "Literal: " << e.value_.lexeme_
        << " [line " << e.line_ << ":" << e.column_ << "]\n";
}

void print_variable(const ast::variable_expr& e, uint32_t level) {
    std::cerr << indent_str(level)
        << "Variable: " << e.name_.lexeme_
        << " [line " << e.line_ << ":" << e.column_ << "]\n";
}

void print_binary(const std::unique_ptr<ast::binary_expr>& e, uint32_t level) {
    std::cerr << indent_str(level)
        << "Binary: " << e->op_.lexeme_
        << " [line " << e->line_ << ":" << e->column_ << "]\n";

    std::cerr << indent_str(level + 1) << "Left:\n";
    print_expression(e->left_, level + 2);

    std::cerr << indent_str(level + 1) << "Right:\n";
    print_expression(e->right_, level + 2);
}

void print_unary(const std::unique_ptr<ast::unary_expr>& e, uint32_t level) {
    std::cerr << indent_str(level)
        << "Unary: " << e->op_.lexeme_
        << " [line " << e->line_ << ":" << e->column_ << "]\n";

    print_expression(e->operand_, level + 1);
}

void print_postfix(const std::unique_ptr<ast::postfix_expr>& e, uint32_t level) {
    std::cerr << indent_str(level)
        << "Postfix: " << e->op_.lexeme_
        << " [line " << e->line_ << ":" << e->column_ << "]\n";

    print_expression(e->operand_, level + 1);
}

void print_call(const std::unique_ptr<ast::call_expr>& e, uint32_t level) {
    std::cerr << indent_str(level)
        << "Call: " << e->callee_.lexeme_
        << " [line " << e->line_ << ":" << e->column_ << "]";

    if (e->args_.empty()) {
        std::cerr << " (no args)\n";
        return;
    }

    std::cerr << "\n";
    for (size_t i = 0; i < e->args_.size(); ++i) {
        std::cerr << indent_str(level + 1) << "Arg " << i << ":\n";
        print_expression(e->args_[i], level + 2);
    }
}

void print_array_literal(const std::unique_ptr<ast::array_literal_expr>& e, uint32_t level) {
	std::cerr << indent_str(level)
		<< "ArrayLiteral: [" << e->elements_.size() << " elements]"
		<< " [line " << e->line_ << ":" << e->column_ << "]\n";
	for (size_t i = 0; i < e->elements_.size(); ++i) {
		std::cerr << indent_str(level + 1) << "Element " << i << ":\n";
		print_expression(e->elements_[i], level + 2);
	}
}

void print_index(const std::unique_ptr<ast::index_expr>& e, uint32_t level) {
	std::cerr << indent_str(level)
		<< "IndexExpr: [line " << e->line_ << ":" << e->column_ << "]\n";
	std::cerr << indent_str(level + 1) << "Object:\n";
	print_expression(e->object_, level + 2);
	std::cerr << indent_str(level + 1) << "Index:\n";
	print_expression(e->index_, level + 2);
}

void print_expression_stmt(const ast::expression_stmt& s, uint32_t level) {
    std::cerr << indent_str(level) << "ExpressionStmt\n";
    print_expression(s.expr_, level + 1);
}

void print_var_declaration(const ast::var_declaration& s, uint32_t level) {
    std::cerr << indent_str(level)
        << "VarDeclaration: " << s.name_.lexeme_
        << " : " << type_name(s.type_);

    if (s.initializer_) {
        std::cerr << " =\n";
        print_expression(*s.initializer_, level + 1);
    } else {
        std::cerr << "\n";
    }
}

void print_block(const ast::block_stmt& s, uint32_t level) {
    std::cerr << indent_str(level)
        << "BlockStmt [" << s.statements_.size() << " statements]\n";

    for (const auto& inner : s.statements_) {
        print_statement(*inner, level + 1);
    }
}

void print_while(const ast::while_stmt& s, uint32_t level) {
    std::cerr << indent_str(level) << "WhileStmt\n";
    std::cerr << indent_str(level + 1) << "Condition:\n";
    print_expression(s.condition_, level + 2);
    std::cerr << indent_str(level + 1) << "Body:\n";
    print_statement(*s.body_, level + 2);
}

void print_for(const ast::for_stmt& s, uint32_t level) {
    std::cerr << indent_str(level) << "ForStmt\n";

    if (s.initializer_) {
        std::cerr << indent_str(level + 1) << "Initializer:\n";
        print_statement(*s.initializer_, level + 2);
    }

    if (s.condition_) {
        std::cerr << indent_str(level + 1) << "Condition:\n";
        print_expression(*s.condition_, level + 2);
    }

    if (s.increment_) {
        std::cerr << indent_str(level + 1) << "Increment:\n";
        print_expression(*s.increment_, level + 2);
    }

    std::cerr << indent_str(level + 1) << "Body:\n";
    print_statement(*s.body_, level + 2);
}

void print_if(const ast::if_stmt& s, uint32_t level) {
    std::cerr << indent_str(level) << "IfStmt\n";

    std::cerr << indent_str(level + 1) << "Condition:\n";
    print_expression(s.condition_, level + 2);

    std::cerr << indent_str(level + 1) << "Then:\n";
    print_statement(*s.then_branch_, level + 2);

    if (s.else_branch_) {
        std::cerr << indent_str(level + 1) << "Else:\n";
        print_statement(*s.else_branch_, level + 2);
    }
}

void print_return(const ast::return_stmt& s, uint32_t level) {
    std::cerr << indent_str(level) << "ReturnStmt";

    if (s.value_) {
        std::cerr << "\n";
        print_expression(*s.value_, level + 1);
    } else {
        std::cerr << " (void)\n";
    }
}

void print_func_declaration(const ast::func_declaration& s, uint32_t level) {
    std::cerr << indent_str(level)
        << "FuncDeclaration: " << s.name_.lexeme_
        << " -> " << type_name(s.return_type_) << "\n";

    std::cerr << indent_str(level + 1) << "Params: ";
    if (s.params_.empty()) {
        std::cerr << "(none)";
    }
    for (const auto& p : s.params_) {
        std::cerr << p.name_.lexeme_ << " : " << type_name(p.type_) << " ";
    }
    std::cerr << "\n";

    std::cerr << indent_str(level + 1) << "Body:\n";
    for (const auto& inner : s.body_->statements_) {
        print_statement(*inner, level + 2);
    }
}

} // anonymous namespace

const char* type_name(const core::type& t) {
    if (t == core::type::int_type())    return "int";
    if (t == core::type::double_type()) return "double";
    if (t == core::type::bool_type())   return "bool";
    if (t == core::type::string_type()) return "string";
    if (t == core::type::void_type())   return "void";
    if (t.is_function())                return "function";
	if (t.is_array())                   return "array";
    if (t.is_unknown())                 return "unknown";
    return "???";
}

void print_expression(const ast::expression& expr, uint32_t level) {
    std::visit(core::overloaded{
        [level](const ast::literal_expr& e) { print_literal(e, level); },
        [level](const ast::variable_expr& e) { print_variable(e, level); },
        [level](const std::unique_ptr<ast::binary_expr>& e) { print_binary(e, level); },
        [level](const std::unique_ptr<ast::unary_expr>& e) { print_unary(e, level); },
        [level](const std::unique_ptr<ast::postfix_expr>& e) { print_postfix(e, level); },
        [level](const std::unique_ptr<ast::call_expr>& e) { print_call(e, level); },
		[level](const std::unique_ptr<ast::array_literal_expr>& e) { print_array_literal(e, level); },
		[level](const std::unique_ptr<ast::index_expr>& e) { print_index(e, level); },
        }, expr);
}

void print_statement(const ast::statement& stmt, uint32_t level) {
    std::visit(core::overloaded{
        [level](const ast::expression_stmt& s) { print_expression_stmt(s, level); },
        [level](const ast::var_declaration& s) { print_var_declaration(s, level); },
        [level](const ast::block_stmt& s) { print_block(s, level); },
        [level](const ast::while_stmt& s) { print_while(s, level); },
        [level](const ast::for_stmt& s) { print_for(s, level); },
        [level](const ast::if_stmt& s) { print_if(s, level); },
        [level](const ast::return_stmt& s) { print_return(s, level); },
        [level](const ast::func_declaration& s) { print_func_declaration(s, level); },
        }, stmt.data_);
}


void print_tokens(const std::vector<core::token>& tokens) {
    std::cerr << "\n";
    std::cerr << "═══════════════════════════════════════════════════════\n";
    std::cerr << "  LEXICAL ANALYSIS\n";
    std::cerr << "═══════════════════════════════════════════════════════\n\n";

    std::cerr << std::left
        << std::setw(20) << "Type"
        << std::setw(20) << "Lexeme"
        << "Location\n";
    std::cerr << std::string(60, '-') << "\n";

    for (const auto& tok : tokens) {
        std::string lexeme(tok.lexeme_);
        if (lexeme.empty()) lexeme = "(empty)";

        std::cerr << std::left
            << std::setw(20) << token_type_name(tok.type_)
            << std::setw(20) << lexeme
            << tok.line_ << ":" << tok.column_ << "\n";
    }

    std::cerr << "\n";
}


void print_ast(const std::vector<std::unique_ptr<ast::statement>>& statements) {
    std::cerr << "\n";
    std::cerr << "═══════════════════════════════════════════════════════\n";
    std::cerr << "  ABSTRACT SYNTAX TREE\n";
    std::cerr << "═══════════════════════════════════════════════════════\n\n";

    for (const auto& stmt : statements) {
        print_statement(*stmt, 0);
        std::cerr << "\n";
    }
}


void print_semantic_info(const std::vector<std::unique_ptr<ast::statement>>&) {
    std::cerr << "\n";
    std::cerr << "═══════════════════════════════════════════════════════\n";
    std::cerr << "  SEMANTIC ANALYSIS (Type Check Passed)\n";
    std::cerr << "═══════════════════════════════════════════════════════\n\n";
    std::cerr << "All types resolved successfully.\n\n";
}


void print_value(const runtime::value& val, uint32_t indent) {
    std::cerr << indent_str(indent);

    auto t = val.type();

    if (t == core::type::int_type()) {
        std::cerr << "int: " << val.as_int().value() << "\n";
    } else if (t == core::type::double_type()) {
        std::cerr << "double: " << val.as_double().value() << "\n";
    } else if (t == core::type::bool_type()) {
        std::cerr << "bool: " << (val.as_bool().value() ? "true" : "false") << "\n";
    } else if (t == core::type::string_type()) {
        std::cerr << "string: \"" << val.as_string().value() << "\"\n";
    } else if (t.is_void()) {
        std::cerr << "void\n";
    } else {
        std::cerr << "unknown\n";
    }
}

void print_execution(const std::string& message, uint32_t indent) {
    std::cerr << indent_str(indent) << "[EXEC] " << message << "\n";
}

} // namespace debug