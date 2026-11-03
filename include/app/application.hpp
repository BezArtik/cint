#pragma once

#include "debug/debug_writer.hpp"

#include <string>
#include <string_view>

namespace cint::app {

/// Точка сборки всего интерпретатора: CLI, чтение файла, запуск пайплайна.
/// main() только конструирует и вызывает run().
class [[nodiscard]] application {
public:
    /// Разбирает аргументы, читает исходник и исполняет его.
    /// Возвращает код возврата процесса. Исключения не покидают run().
    int run(int argc, char** argv);

private:
    int run_pipeline(std::string_view source, cint::debug::debug_writer& writer);

    std::string filename_;
    cint::debug::trace_level trace_mask_ = cint::debug::trace_level::none;
};

}  // namespace cint::app
