// main.cpp

#include "core/error/error_report.hpp"
#include "debug/debug.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "runtime/interpreter.hpp"
#include "semantics/type_check.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

int main(int argc, char* argv[]) {
    try {
        std::cout << "Program execution...\n";
        bool debug = false;
        std::string filename;

        for (int i = 1; i < argc; ++i) {
            std::string_view arg(argv[i]);
            if (arg == "--debug") {
                debug = true;
            } else {
                filename = argv[i];
            }
        }

        if (filename.empty()) {
            std::cerr << "Usage: " << argv[0] << " [--debug] <source_file>\n";
            return 1;
        }

        std::ifstream file(filename);
        if (!file) {
            std::cerr << "Error: cannot open file '" << filename << "'\n";
            return 1;
        }

        const auto start = std::chrono::steady_clock::now();
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string source = buffer.str();

        core::error_reporter reporter(source);
        lexer::lexer lex(source, reporter);
        auto tokens = lex.scan_tokens();
        if (debug) debug::print_tokens(tokens);
        if (reporter.has_error()) {
            std::cerr << "Lexical errors found.\n";
            return 1;
        }

        parser::parser p(tokens, reporter);
        auto ast = p.parse();
        if (debug) debug::print_ast(ast);
        if (reporter.has_error()) {
            std::cerr << "Syntax errors found.\n";
            return 1;
        }

        semantics::type_checker checker(reporter);
        if (reporter.has_error()) {
            std::cerr << "Semantic errors found.\n";
            return 1;
        }

        runtime::interpreter interpreter(reporter, debug);
        interpreter.interpret(ast);
        if (reporter.has_error()) {
            std::cerr << "Runtime errors found.\n";
            return 1;
        }
        const auto finish = std::chrono::steady_clock::now();
        const std::chrono::duration<double> time_program = finish - start;
        std::cout << "Program execution time (" << time_program << ")\n";
        std::cout << "Program finished successfully.\n";

    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
