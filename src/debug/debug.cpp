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
	using tp = core::token_type;
    switch (t) {
    case tp::LEFT_PAREN:    return "LEFT_PAREN";
    case tp::RIGHT_PAREN:   return "RIGHT_PAREN";
    case tp::LEFT_BRACE:    return "LEFT_BRACE";
    case tp::RIGHT_BRACE:   return "RIGHT_BRACE";
	case tp::LEFT_BRACKET:  return "LEFT_BRACKET";
	case tp::RIGHT_BRACKET: return "RIGHT_BRACKET";
    case tp::COMMA:         return "COMMA";
    case tp::DOT:           return "DOT";
    case tp::SEMICOLON:     return "SEMICOLON";
    case tp::PLUS:          return "PLUS";
    case tp::MINUS:         return "MINUS";
    case tp::STAR:          return "STAR";
    case tp::SLASH:         return "SLASH";
    case tp::PERCENT:       return "PERCENT";
    case tp::BANG:          return "BANG";
    case tp::EQUAL:         return "EQUAL";
    case tp::BANG_EQUAL:    return "BANG_EQUAL";
    case tp::EQUAL_EQUAL:   return "EQUAL_EQUAL";
    case tp::GREATER:       return "GREATER";
    case tp::GREATER_EQUAL: return "GREATER_EQUAL";
    case tp::LESS:          return "LESS";
    case tp::LESS_EQUAL:    return "LESS_EQUAL";
    case tp::INCREMENT:     return "INCREMENT";
    case tp::DECREMENT:     return "DECREMENT";
    case tp::PLUS_EQUAL:    return "PLUS_EQUAL";
    case tp::MINUS_EQUAL:   return "MINUS_EQUAL";
    case tp::STAR_EQUAL:    return "STAR_EQUAL";
    case tp::SLASH_EQUAL:   return "SLASH_EQUAL";
    case tp::PERCENT_EQUAL: return "PERCENT_EQUAL";
    case tp::AND:           return "AND";
    case tp::OR:            return "OR";
    case tp::IDENTIFIER:    return "IDENTIFIER";
    case tp::STRING:        return "STRING";
    case tp::NUMBER:        return "NUMBER";
    case tp::KEYWORD:       return "KEYWORD";
    case tp::END_OF_FILE:   return "EOF";
    case tp::UNKNOWN:       return "UNKNOWN";
    default:                return "???";
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
    if (t.is_int())       return "int";
    if (t.is_double())    return "double";
    if (t.is_bool())      return "bool";
    if (t.is_string())    return "string";
    if (t.is_void())      return "void";
    if (t.is_function())  return "function";
	if (t.is_array())     return "array";
    if (t.is_unknown())   return "unknown";
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


void print_semantic_info() {
    std::cerr << "\n";
    std::cerr << "═══════════════════════════════════════════════════════\n";
    std::cerr << "  SEMANTIC ANALYSIS (Type Check Passed)\n";
    std::cerr << "═══════════════════════════════════════════════════════\n\n";
    std::cerr << "All types resolved successfully.\n\n";
}


void print_value(const runtime::value& val, uint32_t indent) {
    std::cerr << indent_str(indent);

    auto t = val.type();

    if (t.is_int()) {
        std::cerr << "int: " << val.as_int().value() << "\n";
    } else if (t.is_double()) {
        std::cerr << "double: " << val.as_double().value() << "\n";
    } else if (t.is_bool()) {
        std::cerr << "bool: " << (val.as_bool().value() ? "true" : "false") << "\n";
    } else if (t.is_string()) {
        std::cerr << "string: \"" << val.as_string().value() << "\"\n";
    } else if (t.is_array()) {
		std::cerr << "array: [size=" << val.array_size() << "]\n";
		auto elements = val.as_array().value();
		for (size_t i = 0; i < elements.size(); ++i) {
			std::cerr << indent_str(indent + 1) << "Element " << i << ":\n";
			print_value(elements[i], indent + 2);
		}
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