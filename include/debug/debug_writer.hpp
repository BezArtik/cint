/**
 * @file include/debug/debug_writer.hpp
 * @brief Конфигурируемый writer для отладочного вывода.
 * @ingroup Debug
 */

#pragma once

#include "debug/trace_level.hpp"

#include <functional>
#include <string_view>

namespace debug {

/**
 * @brief Настраиваемый обработчик отладочного вывода.
 * @ingroup Debug
 *
 * Разделяет политику вывода (куда писать) и фильтрацию (что писать).
 *
 * Состоит из двух компонентов:
 * - **write_**: функция вывода (например, запись в stderr, файл, или no-op)
 * - **mask_**: битовая маска активных уровней трассировки
 *
 * Проверка enabled() выполняется перед каждым выводом — если уровень
 * не активен или функция вывода не задана, сообщение игнорируется.
 *
 * Это позволяет:
 * - В production-сборках передать writer с пустой write_ и none — ноль накладных расходов
 * - В debug-сборках включить нужные уровни без перекомпиляции (через флаги командной строки)
 * - В тестах перенаправить вывод в строку для проверки
 *
 */
struct debug_writer {
    /// Функция вывода сообщения. Если не задана — вывод отключён.
    std::function<void(std::string_view)> write_;

    /// Маска активных уровней трассировки.
    trace_level mask_ = trace_level::none;

    /**
     * @brief Проверяет, нужно ли выводить сообщение данного уровня.
     *
     * @param level Уровень сообщения
     * @return true, если write_ задана и уровень входит в mask_.
     */
    bool enabled(trace_level level) const noexcept { return write_ && has_level(mask_, level); }

    /**
     * @brief Выводит сообщение (без проверки уровня).
     *
     * Вызывающая сторона должна предварительно проверить enabled().
     * Если write_ не задана — вызов безопасен, но ничего не делает.
     *
     * @param msg Сообщение для вывода
     */
    void emit(std::string_view msg) const {
        if (write_) write_(msg);
    }
};

}  // namespace debug
