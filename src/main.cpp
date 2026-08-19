// main.cpp

#include "core/error/error_report.hpp"
#include "core/memory/arena.hpp"
#include "core/symbol/symbol_registry.hpp"
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

namespace {

struct flag_info {
    std::string_view name_;
    debug::trace_level level_;
};

// clang-format off
constexpr std::array flag_table = {
    flag_info{"--debug", debug::trace_level::all},         
    flag_info{"--trace=tokens", debug::trace_level::tokens},
    flag_info{"--trace=ast", debug::trace_level::ast},     
    flag_info{"--trace=exec", debug::trace_level::execution},
    flag_info{"--trace=returns", debug::trace_level::returns},
    flag_info{"--trace=all", debug::trace_level::all},
};
// clang-format on

struct options {
    bool debug_ = false;
    std::string filename_;
    debug::trace_level trace_mask_ = debug::trace_level::none;
};

options parse_args(int argc, char* argv[]) {
    options opts;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);

        if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options] <source_file>\n"
                      << "Options:\n"
                      << "  --debug              Enable all debug output\n"
                      << "  --trace=<level>      Enable specific trace level:\n"
                      << "                         tokens | ast | exec | returns | all\n"
                      << "  --help               Show this help\n";
            std::exit(0);
        }

        auto&& it = std::ranges::find(flag_table, arg, &flag_info::name_);
        if (it != flag_table.end()) {
            opts.debug_ = true;
            opts.trace_mask_ = opts.trace_mask_ | it->level_;
            continue;
        }

        opts.filename_ = argv[i];
    }
    return opts;
}

struct run_config {
    std::string_view source_;
    const debug::debug_writer& writer_;
};

int run_program(const run_config& config) {
    core::arena arena;
    core::arena_memory_resource mr{arena};
    core::error_reporter reporter{config.source_};

    lexer lex{config.source_, reporter, mr};
    auto tokens = lex.scan_tokens();

    debug::print_tokens(config.writer_, tokens);
    if (reporter.has_error()) {
        std::cerr << "Lexical errors found.\n";
        return 1;
    }

    parser p{tokens, reporter, arena, mr};
    auto&& ast = p.parse();

    debug::print_ast(config.writer_, ast);
    if (reporter.has_error()) {
        std::cerr << "Syntax errors found.\n";
        return 1;
    }

    auto&& registry = core::symbol_registry::build(ast);

    type_checker checker{reporter, registry};
    if (!checker.check(ast)) {
        std::cerr << "Semantic errors found.\n";
        return 1;
    }

    interpreter interpreter{reporter, registry, config.writer_};
    interpreter.interpret(ast);

    if (reporter.has_error()) {
        std::cerr << "Runtime errors found.\n";
        return 1;
    }

    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        auto&& opts = parse_args(argc, argv);

        if (opts.filename_.empty()) {
            std::cerr << "Usage: " << argv[0] << " [options] <source_file>\n"
                      << "Try '" << argv[0] << " --help' for more information.\n";
            return 1;
        }

        std::ifstream file{opts.filename_};
        if (!file) {
            std::cerr << "Error: cannot open file '" << opts.filename_ << "'\n";
            return 1;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        auto&& source = buffer.str();

        debug::debug_writer writer{
            opts.debug_ ? [](std::string_view msg) { std::cerr << msg; } : [](std::string_view) {}, opts.trace_mask_};
        run_config config{source, writer};

        auto&& start = std::chrono::steady_clock::now();
        auto&& exit_code = run_program(config);
        auto&& finish = std::chrono::steady_clock::now();

        const std::chrono::duration<double> elapsed{finish - start};
        std::cout << "\nProgram execution time (" << elapsed << ")\n";

        if (exit_code == 0) std::cout << "Program finished successfully.\n";

        return exit_code;

    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << "\n";
        return 1;
    }
}
