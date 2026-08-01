/**
 * @file include/core/utils/symbol_registry.hpp
 * @brief Реестр символов — функции, builtin-функции, структуры.
 * @ingroup CoreUtils
 */

#pragma once

#include "ast/statement.hpp"
#include "core/token/type.hpp"
#include "core/utils/builtins.hpp"

#include <span>
#include <string_view>
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
 * Каждая запись хранит:
 * - Имя символа
 * - Тип (для функций — сигнатура, для структур — структурный тип)
 * - Указатель на реализацию (AST-узел или указатель на builtin-функцию)
 *
 * Строится **один раз** перед семантическим анализом статическим методом
 * build(). После построения реестр не изменяется.
 *
 * Поиск — бинарный поиск по отсортированному вектору имён (O(log n)).
 *
 * Особенности:
 * - **Builtin-функции** можно переопределить пользовательской функцией
 *   (запись о builtin заменяется записью о пользовательской функции)
 * - **Структуры** разрешаются рекурсивно: resolve_type() заменяет
 *   упоминания структур их полным определением
 * - **Порядок объявлений** не важен: структуры могут ссылаться друг на друга,
 *   так как resolve_type() вызывается после построения реестра
 *
 */
class symbol_registry {
public:
    /// Указатель на узел пользовательской функции в AST.
    using func_ptr = const ast::func_declaration*;

    /// Указатель на узел структуры в AST.
    using struct_ptr = const ast::struct_declaration*;

    /**
     * @brief Информация о символе (одна из трёх альтернатив).
     *
     * variant выбран вместо иерархии классов для:
     * - Компактного хранения (размер variant = размер наибольшей альтернативы)
     * - Явной проверки типа через std::holds_alternative / std::visit
     */
    using info_variant = std::variant<func_ptr, builtin_fn_ptr, struct_ptr>;

    /**
     * @brief Запись реестра: имя, тип, информация о реализации.
     */
    struct entry {
        std::string_view name_;  ///< Имя символа
        type type_;  ///< Тип символа (сигнатура функции или структурный тип)
        info_variant info_;  ///< Информация о реализации
    };

    /**
     * @brief Строит реестр из AST и списка builtin-функций.
     *
     * Порядок построения:
     * 1. Добавляются все builtin-функции (могут быть переопределены)
     * 2. Обходятся объявления верхнего уровня AST
     * 3. Записи сортируются по имени для бинарного поиска
     *
     * @param ast      Список объявлений верхнего уровня
     * @param builtins Список встроенных функций (core::builtins)
     * @return Готовый реестр символов.
     */
    static symbol_registry build(std::span<const ast::node<ast::statement>> ast, std::span<const builtin_def> builtins);

    /**
     * @brief Ищет символ по имени.
     *
     * Бинарный поиск по отсортированному вектору записей.
     *
     * @param name Имя символа
     * @return Указатель на запись или nullptr, если символ не найден.
     */
    const entry* find(std::string_view name) const noexcept;

    /**
     * @brief Разрешает тип, заменяя упоминания структур их определениями.
     *
     * Рекурсивно обходит тип и для каждого struct-типа ищет
     * его полное определение в реестре. Если определение не найдено —
     * возвращает unknown_type.
     *
     * Пример:
     * - Тип `struct Point` без полей → полный тип `Point {x: int, y: int}`
     * - Тип `Point[]` → `Point {x: int, y: int}[]`
     *
     * @param t Тип для разрешения (может содержать неполные struct-типы)
     * @return Полный тип с разрешёнными структурами или unknown_type при ошибке.
     */
    type resolve_type(const type& t) const;

private:
    symbol_registry() = default;

    /// Добавляет builtin-функции в реестр (до AST-объявлений).
    void add_builtins(std::span<const builtin_def> builtins);

    /**
     * @brief Добавляет AST-объявление в реестр.
     *
     * Для функций: если builtin с таким именем уже существует — заменяет его.
     * Для структур: добавляет новую запись.
     * Другие типы объявлений игнорируются.
     */
    void add_ast_entry(const ast::node<ast::statement>& stmt);

    std::vector<entry> entries_;  ///< Отсортированный по имени вектор записей
};

}  // namespace core
