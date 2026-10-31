// main.cpp

#include "core/error/error_report.hpp"
#include "core/memory/arena.hpp"
#include "core/symbol/symbol_registry.hpp"
#include "debug/debug_writer.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "runtime/interpreter.hpp"
#include "semantics/type_check.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

namespace {

struct flag_info {
    std::string_view name_;
    cint::debug::trace_level level_;
};

// clang-format off
constexpr std::array flag_table = {
    flag_info{"--debug", cint::debug::trace_level::all},         
    flag_info{"--trace=tokens", cint::debug::trace_level::tokens},
    flag_info{"--trace=ast", cint::debug::trace_level::ast},     
    flag_info{"--trace=exec", cint::debug::trace_level::execution},
    flag_info{"--trace=returns", cint::debug::trace_level::returns},
    flag_info{"--trace=all", cint::debug::trace_level::all},
};
// clang-format on

struct options {
    bool debug_ = false;
    std::string filename_;
    cint::debug::trace_level trace_mask_ = cint::debug::trace_level::none;
};

options parse_args(int argc, char** argv) {
    auto opts = options{};
    for (int i = 1; i < argc; ++i) {
        const auto arg = std::string_view(argv[i]);

        if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options] <source_file>\n"
                      << "Options:\n"
                      << "  --debug              Enable all debug output\n"
                      << "  --trace=<level>      Enable specific trace level:\n"
                      << "                         tokens | ast | exec | returns | all\n"
                      << "  --help               Show this help\n";
            std::exit(0);
        }

        const auto it = std::ranges::find(flag_table, arg, &flag_info::name_);
        if (it != flag_table.end()) {
            opts.debug_ = true;
            opts.trace_mask_ = opts.trace_mask_ | it->level_;
            continue;
        }

        opts.filename_ = arg;
    }
    return opts;
}

struct run_config {
    std::string_view source_;
    cint::debug::debug_writer& writer_;
};

int run_program(const run_config& config) {
    auto arena = cint::core::arena{};
    auto mr = cint::core::arena_memory_resource{arena};
    auto reporter = cint::core::error_reporter{config.source_};

    auto lex = cint::lexer{config.source_, reporter, mr};
    const auto tokens = lex.scan_tokens();

    config.writer_.print_tokens(tokens);
    if (reporter.has_error()) {
        std::cerr << "Lexical errors found.\n";
        return 1;
    }

    auto pr = cint::parser{tokens, reporter, arena, mr};
    const auto ast = pr.parse();

    config.writer_.print_ast(ast);
    if (reporter.has_error()) {
        std::cerr << "Syntax errors found.\n";
        return 1;
    }

    const auto registry = cint::core::symbol_registry::build(ast, reporter);

    auto checker = cint::type_checker{reporter, registry};
    if (!checker.check(ast)) {
        std::cerr << "Semantic errors found.\n";
        return 1;
    }

    auto interp = cint::interpreter{reporter, registry, config.writer_};
    interp.interpret(ast);

    if (reporter.has_error()) {
        std::cerr << "Runtime errors found.\n";
        return 1;
    }

    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const auto opts = parse_args(argc, argv);

        if (opts.filename_.empty()) {
            std::cerr << "Usage: " << argv[0] << " [options] <source_file>\n"
                      << "Try '" << argv[0] << " --help' for more information.\n";
            return 1;
        }

        auto file = std::ifstream{opts.filename_};
        if (!file) {
            std::cerr << "Error: cannot open file '" << opts.filename_ << "'\n";
            return 1;
        }

        const auto source = std::string{std::istreambuf_iterator(file), {}};

        cint::debug::debug_writer writer{
            opts.debug_ ? [](std::string_view msg) { std::cerr << msg; } : [](std::string_view) {}, opts.trace_mask_};
        run_config config{source, writer};

        const auto start = std::chrono::steady_clock::now();
        const auto exit_code = run_program(config);
        const auto finish = std::chrono::steady_clock::now();

        const auto elapsed = std::chrono::duration<double>{finish - start};
        std::cout << "\nProgram execution time (" << elapsed << ")\n";

        if (exit_code == 0) std::cout << "Program finished successfully.\n";

        return exit_code;

    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << "\n";
        return 1;
    }
}
