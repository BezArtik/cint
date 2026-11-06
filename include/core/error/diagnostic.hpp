/**
 * @file include/core/error/diagnostic.hpp
 * @brief Диагностическое сообщение и накопитель ошибок.
 * @ingroup CoreError
 *
 */

#pragma once
#include "core/error/error_codes.hpp"
#include "core/token/token.hpp"

#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cint::core {

/**
 * @brief Одно диагностическое сообщение.
 */
struct diagnostic {
    error_code code_;      ///< Код ошибки
    location loc_;         ///< Позиция в исходном коде
    std::string message_;  ///< Отформатированный текст
};

/**
 * @brief Накопитель диагностических сообщений.
 *
 */
class diagnostic_sink {
public:
    /**
     * @brief Регистрирует ошибку.
     *
     * @param loc  Позиция в исходном коде
     * @param code Код ошибки
     * @param args Аргументы для плейсхолдеров {} в шаблоне сообщения
     */
    template <typename... Args>
    void report(location loc, error_code code, Args&&... args) {
        items_.emplace_back(code, loc, format_message(code, std::forward<Args>(args)...));
    }

    /// Были ли зарегистрированы ошибки.
    bool has_errors() const noexcept { return !items_.empty(); }

    /**
     * @brief Была ли зарегистрирована ошибка с указанным кодом.
     *
     */
    bool has_error(error_code code) const noexcept {
        return std::ranges::find(items_, code, &diagnostic::code_) != items_.end();
    }

    /// Итераторы для доступа к диагностике.
    auto begin() const noexcept { return items_.begin(); }
    auto end() const noexcept { return items_.end(); }

    /// Очистить накопленное (между фазами).
    void clear() noexcept { items_.clear(); }

private:
    std::vector<diagnostic> items_;

    /**
     * @brief Форматирует сообщение об ошибке.
     *
     * Подставляет аргументы в шаблон сообщения из error_table.
     * Если аргументов нет — возвращает сообщение как есть.
     *
     * @tparam Args Типы аргументов
     * @param code Код ошибки
     * @param args Аргументы для форматирования
     * @return Отформатированная строка.
     */
    template <typename... Args>
    auto format_message(error_code code, Args&&... args) {
        const auto format = error_table[static_cast<std::size_t>(code)].format_;
        if constexpr (sizeof...(Args) == 0) return std::string{format};
        return std::vformat(format, std::make_format_args(args...));
    }
};

}  // namespace cint::core
