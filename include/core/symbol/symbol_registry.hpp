/**
 * @file include/core/symbol/symbol_registry.hpp
 * @brief Реестр символов — функции, builtin-функции, структуры.
 * @ingroup CoreUtils
 */

#pragma once

#include "ast/statement.hpp"
#include "core/builtins/builtins.hpp"
#include "core/error/error_report.hpp"
#include "core/type/type.hpp"

#include <span>
#include <string_view>
#include <unordered_set>
#include <variant>
#include <vector>

namespace core {

/**
 * @brief Реестр всех объявленных символов программы.
 * @ingroup CoreUtils
 *
 * Содержит информацию о каждом именованном символе верхнего уровня:
 * - **Пользовательские функции** (func_declaration)
 * - **Builtin-функции** (встроенные в интерпретатор)
 * - **Структуры** (struct_declaration)
 *
 */
class symbol_registry {
    struct entry;
    /// Тип списка записей.
    using entries_t = std::vector<entry>;
    /// Итератор на запись внутри списка.
    using const_iterator = typename entries_t::const_iterator;

public:
    /// Указатель на узел пользовательской функции в AST.
    using func_ptr = const ast::func_declaration_stmt*;

    /// Указатель на узел структуры в AST.
    using struct_ptr = const ast::struct_declaration_stmt*;

    /// Указатель на builtin-функцию.
    using builtin_func_ptr = core::builtin_fn_ptr;

    /**
     * @brief Строит реестр из AST и списка builtin-функций.
     *
     * Порядок построения:
     * 1. Добавляются все builtin-функции.
     * 2. Обходятся объявления верхнего уровня AST.
     *
     * @param ast      Список объявлений верхнего уровня
     * @param reporter Обработчик ошибок
     * @return Готовый реестр символов.
     */
    static symbol_registry build(std::span<const ast::statement> ast, error_reporter& reporter);

    /// @name Итераторы
    /// @{

    auto begin() const noexcept { return entries_.begin(); }
    auto end() const noexcept { return entries_.end(); }

    /// @}

    /**
     * @brief Находит запись по имени.
     * @param name Имя символа
     * @return Итератор на запись или end(), если не найдена.
     */
    const_iterator find(std::string_view name) const noexcept;

    /**
     * @brief Разрешает тип, заменяя упоминания структур их определениями.
     *
     * Рекурсивно обходит тип и для каждого struct-типа ищет
     * его полное определение в реестре. Если определение не найдено —
     * возвращает unknown_type.
     *
     * @param t Тип для разрешения (может содержать неполные struct-типы)
     * @return Полный тип с разрешёнными структурами или unknown_type при ошибке.
     */
    type resolve_type(const type& t) const;

private:
    /**
     * @brief Запись реестра: имя, тип, информация о реализации.
     */
    struct entry {
        std::string_view name_;                                      ///< Имя символа
        type type_;                                                  ///< Тип символа
        std::variant<func_ptr, builtin_func_ptr, struct_ptr> info_;  ///< Информация о реализации
    };

    symbol_registry() = default;

    /**
     * @brief Добавляет AST-объявление в реестр.
     */
    void add_ast_entry(const ast::statement& stmt, error_reporter& reporter);
    type resolve_type_impl(const type& t, std::unordered_set<std::string_view>& resolving) const;

    entries_t entries_;
};

}  // namespace core
