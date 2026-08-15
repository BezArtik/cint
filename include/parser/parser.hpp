/**
 * @file include/parser/parser.hpp
 * @brief Синтаксический анализатор — второй этап конвейера, строит AST из токенов.
 * @ingroup Parser
 *
 * @defgroup Parser Синтаксический анализ
 * @brief Построение AST методом рекурсивного спуска.
 */

#pragma once
#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "core/error/error_report.hpp"
#include "core/token/token.hpp"
#include "core/utils/arena.hpp"

#include <memory_resource>
#include <span>

/**
 * @brief Синтаксический анализатор, строящий AST из
 * входной последовательности токенов
 */
class parser {
public:
    /**
     * @brief Конструктор синтаксического анализатора.
     *
     * @param tokens   Список токенов от лексера (должен завершаться END_OF_FILE)
     * @param reporter Обработчик ошибок
     * @param arena    Арена для размещения узлов AST
     * @param mr       Memory resource для контейнеров списков
     *
     * @note Парсер не копирует токены, а хранит span на переданный список.
     *       Вызывающий код должен гарантировать время жизни списка токенов.
     */
    parser(std::span<const core::token> tokens, core::error_reporter& reporter, core::arena& arena,
           core::arena_memory_resource& mr);

    /**
     * @brief Выполняет синтаксический разбор программы.
     *
     * Основная точка входа в парсер. Разбирает токены до конца файла,
     * формируя список объявлений верхнего уровня.
     *
     * Алгоритм:
     * 1. В цикле вызывает declaration() для каждого объявления
     * 2. Пропускает ошибочные конструкции через synchronize()
     * 3. Завершается при достижении END_OF_FILE
     *
     * @return Список AST-узлов верхнего уровня (объявления функций,
     *         переменных, структур).
     *
     * @throws core::parse_error при невозможности восстановления после ошибки.
     *
     * @note Метод можно вызывать только один раз. Повторный вызов
     *       приведёт к неопределённому поведению.
     */
    ast::stmt_list parse();

private:
    /**
     * @brief Потребляет текущий токен и переходит к следующему.
     * @return Ссылка на потреблённый токен.
     */
    const core::token& advance() noexcept;

    /**
     * @brief Возвращает текущий токен без продвижения.
     * @return Ссылка на текущий токен.
     */
    const core::token& peek() const noexcept;

    /**
     * @brief Возвращает предыдущий потреблённый токен.
     * @pre Был вызван хотя бы один advance().
     * @return Ссылка на предыдущий токен.
     */
    const core::token& prev() const noexcept;

    /**
     * @brief Проверяет, достигнут ли конец потока токенов.
     * @return true, если текущий токен — END_OF_FILE.
     */
    bool is_at_end() const noexcept;

    /**
     * @brief Проверяет тип текущего токена.
     * @param type Ожидаемый тип.
     * @return true, если текущий токен имеет указанный тип.
     */
    bool check(core::token_type type) const noexcept;

    /**
     * @brief Проверяет и потребляет токен, если его тип входит в список.
     *
     * Используется для разбора альтернатив (например, типов данных
     * или операторов сравнения).
     *
     * @param types Список допустимых типов токенов.
     * @return true, если токен совпал и был потреблён.
     */
    bool match(std::initializer_list<core::token_type> types) noexcept;

    /**
     * @brief Проверяет и потребляет токен требуемого типа.
     *
     * Если тип не совпадает, генерирует синтаксическую ошибку и
     * выбрасывает core::parse_error.
     *
     * @param type Ожидаемый тип токена.
     * @param code Код ошибки при несовпадении.
     * @return Ссылка на потреблённый токен.
     * @throws core::parse_error при несовпадении типа.
     */
    const core::token& consume(core::token_type type, core::error_code code);

    /**
     * @brief Разбирает объявление верхнего уровня.
     *
     * Диспетчеризует:
     * - `struct` → struct_declaration()
     * - Тип + идентификатор + `(` → func_declaration()
     * - Тип + идентификатор → var_declaration()
     * - Иначе → statement()
     *
     * @return Узел AST или nullopt при ошибке.
     */
    std::optional<ast::statement> declaration();

    /**
     * @brief Разбирает инструкцию.
     *
     * Обрабатывает:
     * - `while` → while_statement()
     * - `for`   → for_statement()
     * - `if`    → if_statement()
     * - `return`→ return_statement()
     * - `{`     → block_statement()
     * - Иначе   → expression + `;`
     *
     * @return Узел инструкции.
     */
    ast::statement statement();

    /**
     * @brief Разбирает объявление переменной.
     *
     * Синтаксис: `type identifier [= expression];`
     *
     * @param type Тип переменной (уже разобран)
     * @param name Токен имени переменной (уже потреблён)
     * @return Узел var_declaration.
     */
    ast::statement var_declaration(core::type type, const core::token& name);

    /**
     * @brief Разбирает объявление функции.
     *
     * Синтаксис: `return_type identifier (params) { body }`
     *
     * @param return_type Тип возвращаемого значения (уже разобран)
     * @param name        Токен имени функции (уже потреблён)
     * @return Узел func_declaration.
     */
    ast::statement func_declaration(core::type return_type, const core::token& name);

    /**
     * @brief Разбирает объявление структуры.
     *
     * Синтаксис: `struct identifier { field_type field_name; ... };`
     *
     * @param name Токен имени структуры (уже потреблён)
     * @return Узел struct_declaration.
     */
    ast::statement struct_declaration(const core::token& name);

    /**
     * @brief Разбирает цикл while.
     *
     * Синтаксис: `while (condition) body`
     *
     * @return Узел while_stmt.
     */
    ast::statement while_statement();

    /**
     * @brief Разбирает цикл for.
     *
     * Синтаксис: `for (init; condition; increment) body`
     *
     * @return Узел for_stmt.
     */
    ast::statement for_statement();

    /**
     * @brief Разбирает условную инструкцию if/else.
     *
     * Синтаксис: `if (condition) then_branch [else else_branch]`
     *
     * @return Узел if_stmt.
     */
    ast::statement if_statement();

    /**
     * @brief Разбирает блок инструкций в фигурных скобках.
     *
     * Синтаксис: `{ declaration* }`
     *
     * @return Узел block_stmt.
     */
    ast::statement block_statement();

    /**
     * @brief Разбирает инструкцию возврата.
     *
     * Синтаксис: `return [expression];`
     *
     * @return Узел return_stmt.
     */
    ast::statement return_statement();

    /**
     * @brief Разбирает выражение с учётом приоритета.
     *
     * Ядро Pratt-парсера. Выполняет цикл разбора инфиксных операторов,
     * пока их приоритет >= переданного минимального приоритета.
     *
     * @param precedence Минимально допустимый приоритет оператора.
     * @return Разобранное выражение.
     */
    ast::expression parse_expression(int8_t precedence);

    /**
     * @brief Точка входа в разбор выражений.
     *
     * Эквивалент parse_expression(0).
     *
     * @return Разобранное выражение.
     */
    ast::expression expression();

    /**
     * @brief Разбирает присваивание или выражение.
     *
     * Проверяет, является ли следующий оператор оператором присваивания
     * (`=`, `+=`, `-=`, и т.д.). Если да — строит assignment_expr,
     * иначе возвращает результат expression().
     *
     * @note Присваивание правоассоциативно: `a = b = c` → `a = (b = c)`
     *
     * @return Разобранное выражение.
     */
    ast::expression assignment();

    /**
     * @brief Разбирает унарное выражение.
     *
     * Обрабатывает префиксные операторы: `-`, `!`, `++`, `--`.
     * Рекурсивно вызывает себя для поддержки выражений вида `!!x`.
     *
     * @return Разобранное выражение.
     */
    ast::expression unary();

    /**
     * @brief Разбирает постфиксное выражение.
     *
     * После разбора первичного выражения (primary) в цикле применяет:
     * - Индексацию: `[expression]`
     * - Доступ к полю: `.identifier`
     * - Постфиксные ++/--
     *
     * @return Разобранное выражение.
     */
    ast::expression postfix();

    /**
     * @brief Разбирает список инициализации.
     *
     * Синтаксис: `{ expression, expression, ... }`
     *
     * @return Узел initializer_list_expr.
     */
    ast::expression initializer_list();

    /**
     * @brief Обрабатывает escape-последовательности в строке.
     *
     * Поддерживаемые последовательности:
     * - \\n, \\t, \\r, \\\\, \\", \\0
     *
     * @param raw       Исходная строка (без внешних кавычек)
     * @param start_loc Позиция начала строки (для сообщений об ошибках)
     * @return Обработанная строка с раскрытыми escape-последовательностями.
     */
    std::string process_escape_sequences(std::string_view raw, core::location start_loc);

    /**
     * @brief Разбирает первичное (атомарное) выражение.
     *
     * Обрабатывает:
     * - Литералы: числа, строки, true/false
     * - Идентификаторы (с опциональным вызовом функции)
     * - Группировку: `(expression)`
     *
     * @return Разобранное выражение.
     * @throws core::parse_error если не удалось разобрать выражение.
     */
    ast::expression primary();

    /**
     * @brief Завершает разбор вызова функции.
     *
     * Вызывается после того, как `primary()` обнаружил
     * `identifier(`. Разбирает список аргументов в скобках.
     *
     * @param callee Токен имени функции (уже потреблён)
     * @return Узел call_expr.
     */
    ast::expression finish_call(const core::token& callee);

    /**
     * @brief Завершает разбор операции индексации.
     *
     * Вызывается после того, как `postfix()` обнаружил `[`.
     * Разбирает индексное выражение и закрывающую скобку.
     *
     * @param object Выражение-объект индексации (уже разобрано)
     * @return Узел index_expr.
     */
    ast::expression finish_index(ast::expression object);

    /**
     * @brief Разбирает параметр функции.
     *
     * Синтаксис: `type identifier`
     *
     * @return Структура func_param с типом и именем параметра.
     */
    ast::func_param parse_param();

    /**
     * @brief Разбирает спецификатор типа.
     *
     * Обрабатывает:
     * - Примитивные типы: `int`, `double`, `bool`, `string`, `void`
     * - Пользовательские типы: `struct identifier`
     *
     * @return Объект core::type.
     * @throws core::parse_error если тип не распознан.
     */
    core::type parse_type();

    /**
     * @brief Разбирает размерности массива после типа.
     *
     * Обрабатывает цепочку `[]` после базового типа, строя
     * вложенный array_type. Пример: `int[3][5]` → array(array(int, 3), 5)
     *
     * @param base_type Базовый тип (до скобок)
     * @return Тип с учётом размерностей массива.
     */
    core::type parse_array_dimensions(core::type base_type);

    /**
     * @brief Восстанавливает состояние парсера после синтаксической ошибки.
     *
     * Пропускает токены до безопасной точки продолжения разбора:
     * - Конец инструкции (`;`)
     * - Закрывающая фигурная скобка (`}`) с учётом вложенности
     * - Начало нового объявления (ключевое слово)
     *
     * Это позволяет обнаруживать несколько ошибок за один проход,
     * а не останавливаться на первой.
     *
     * @note Метод не выбрасывает исключений — после его вызова парсер
     *       готов продолжить разбор со следующей инструкции.
     */
    void synchronize();

    std::span<const core::token> tokens_;  ///< Поток токенов от лексера
    core::error_reporter& reporter_;       ///< Обработчик ошибок
    size_t current_ = 0;                   ///< Индекс текущего токена
    core::arena& arena_;                   ///< Арена для размещения узлов AST
    core::arena_memory_resource& mr_;      ///< Memory resource для списков
    std::pmr::memory_resource* temp_mr_;  ///< Промежуточный memory_resource для хранения списков,
                                          ///  которые не пойдут в AST
};
