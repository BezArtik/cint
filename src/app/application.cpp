// app/application.cpp
#include "app/application.hpp"

#include "core/error/diagnostic.hpp"
#include "core/error/diagnostic_renderer.hpp"
#include "core/memory/arena.hpp"
#include "core/symbol/symbol_registry.hpp"
#include "core/utils/literal_pool.hpp"
#include "debug/debug_writer.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "runtime/interpreter.hpp"

#include <CLI/CLI.hpp>
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <print>
#include <ranges>
#include <string>
#include <string_view>

namespace cint::app {

namespace {

struct trace_info {
    std::string_view name_;
    debug::trace_level level_;
};

// clang-format off
constexpr std::array trace_levels = {
    trace_info{"tokens", debug::trace_level::tokens},  
    trace_info{"ast", debug::trace_level::ast},
    trace_info{"exec", debug::trace_level::execution}, 
    trace_info{"returns", debug::trace_level::returns},
    trace_info{"all", debug::trace_level::all},
};
// clang-format on

}  // namespace

int run(int argc, char** argv) {
    auto cli = CLI::App{"Cint — interpreter", "cint"};

    auto debug = bool{};
    auto trace_values = std::vector<std::string>{};
    auto filename = std::string{};
    auto trace_mask = debug::trace_level::none;

    cli.add_flag("--debug", debug, "Enable all levels of debug output");
    cli.add_option("--trace", trace_values, "Trace level")
        ->check(CLI::IsMember({"tokens", "ast", "exec", "returns", "all"}));
    cli.add_option("source_file", filename, "The source file for execution")->required();

    try {
        cli.parse(argc, argv);
    } catch (const CLI::ParseError& e) { return cli.exit(e); }

    if (debug) trace_mask = debug::trace_level::all;
    for (auto&& name : trace_values) {
        const auto it = std::ranges::find(trace_levels, name, &trace_info::name_);
        trace_mask = trace_mask | it->level_;
    }

    try {
        auto file = std::ifstream{filename};
        if (!file) {
            std::println(std::cerr, "Error: cannot open file '{}'", filename);
            return EXIT_FAILURE;
        }

        const auto source = std::string{std::istreambuf_iterator{file}, {}};
        auto writer = debug::debug_writer{trace_mask};
        auto arena = core::arena{};
        auto literals = core::literal_pool{};
        auto sink = core::diagnostic_sink{};
        auto renderer = core::diagnostic_renderer{source};

        auto lex = lexer{source, sink, arena};
        const auto tokens = lex.scan_tokens();
        writer.print_tokens(tokens);
        if (sink.has_errors()) {
            renderer.render(sink.begin(), sink.end());
            return EXIT_FAILURE;
        }

        auto pr = parser{tokens, sink, arena, literals};
        const auto ast = pr.parse();
        writer.print_ast(ast);
        if (sink.has_errors()) {
            renderer.render(sink.begin(), sink.end());
            return EXIT_FAILURE;
        }

        const auto registry = core::symbol_registry{ast, sink};
        if (sink.has_errors()) {
            renderer.render(sink.begin(), sink.end());
            return EXIT_FAILURE;
        }

        try {
            const auto interp = interpreter{ast, registry, writer, literals};
        } catch (const runtime_error& e) {
            const auto d = core::diagnostic{e.code_, e.loc_, e.message_};
            const auto single_error = std::views::single(d);
            renderer.render(single_error.begin(), single_error.end());
            return EXIT_FAILURE;
        }

        std::println("Program finished successfully.");
        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        std::println(std::cerr, "Fatal error: {}", e.what());
        return EXIT_FAILURE;
    }
}

}  // namespace cint::app
