/**
 * @file include/core/utils/arena.hpp
 * @brief Арена-аллокатор и адаптер для std::pmr.
 * @ingroup Core
 *
 * @defgroup CoreUtils Утилиты
 * @brief Вспомогательные классы и структуры данных ядра интерпретатора.
 */

#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace core {

/**
 * @brief Арена-аллокатор с поддержкой деструкторов.
 * @ingroup CoreUtils
 *
 * Реализует стратегию **region-based memory management**:
 * - Память выделяется крупными блоками (64 КБ по умолчанию)
 * - Отдельные объекты не освобождаются — вся арена очищается разом
 * - Для нетривиально разрушаемых объектов регистрируются деструкторы
 *
 * Преимущества для интерпретатора:
 * - **Производительность**: выделение памяти — O(1) (инкремент указателя)
 * - **Локальность данных**: последовательные выделения размещаются рядом
 * - **Детерминированное освобождение**: reset() вызывает все деструкторы
 *   и освобождает все блоки одной операцией
 *
 * Потокобезопасность: **отсутствует**. Рассчитана на использование
 * в одном потоке (весь интерпретатор однопоточный).
 *
 * @invariant Все указатели, возвращённые allocate()/allocate_raw(),
 *            валидны до вызова reset() или разрушения арены.
 *
 */
class arena {
    /// Размер блока по умолчанию: 64 КБ.
    static constexpr size_t BLOCK_SIZE = 64 * 1024;

public:
    arena() = default;

    /// @name Копирование и перемещение запрещены
    /// @{
    arena(const arena&) = delete;
    arena& operator=(const arena&) = delete;
    arena(arena&&) = delete;
    arena& operator=(arena&&) = delete;
    /// @}

    /// Вызывает reset() для очистки всех ресурсов.
    ~arena();

    /**
     * @brief Размещает объект типа T в арене.
     *
     * Выделяет память с учётом выравнивания T и конструирует объект
     * на месте с переданными аргументами. Если у T нетривиальный
     * деструктор — регистрирует его для вызова при reset().
     *
     * @tparam T    Тип размещаемого объекта
     * @tparam Args Типы аргументов конструктора T
     * @param args  Аргументы конструктора T
     * @return Указатель на сконструированный объект.
     */
    template <typename T, typename... Args>
    T* allocate(Args&&... args) {
        auto* ptr = allocate_raw(sizeof(T), alignof(T));
        auto* obj = ::new (ptr) T(std::forward<Args>(args)...);

        if constexpr (!std::is_trivially_destructible_v<T>) destructors_.push_back([obj] { obj->~T(); });

        return obj;
    }

    /**
     * @brief Выделяет сырую память заданного размера.
     *
     * Если запрошенный размер больше BLOCK_SIZE — выделяет отдельный
     * блок нужного размера (не тратит стандартные блоки).
     *
     * @param size      Размер в байтах
     * @param alignment Выравнивание (степень двойки)
     * @return Указатель на выровненную память.
     */
    void* allocate_raw(size_t size, size_t alignment);

    /**
     * @brief Освобождает всю память арены.
     *
     * Порядок действий:
     * 1. Вызывает все зарегистрированные деструкторы в обратном порядке
     * 2. Освобождает все блоки памяти
     * 3. Сбрасывает счётчики
     *
     * @note После reset() все ранее возвращённые указатели становятся невалидными.
     */
    void reset() noexcept;

private:
    void allocate_block();
    static char* align_ptr(char* ptr, size_t alignment) noexcept;

    // Данные
    std::vector<std::unique_ptr<char[]>> blocks_;     ///< Выделенные блоки памяти
    std::vector<std::function<void()>> destructors_;  ///< Зарегистрированные деструкторы
    char* current_ = nullptr;                         ///< Текущая позиция в активном блоке
    char* end_ = nullptr;                             ///< Конец активного блока
};

/**
 * @brief Адаптер арены для std::pmr-контейнеров.
 * @ingroup CoreUtils
 *
 * Позволяет использовать arena с любым стандартным контейнером,
 * принимающим polymorphic_allocator (std::pmr::vector, std::pmr::string).
 *
 * Деаллокация — no-op: память освобождается только при reset() арены.
 *
 */
class arena_memory_resource : public std::pmr::memory_resource {
public:
    /**
     * @brief Конструктор.
     * @param arena Арена, в которой будет выделяться память.
     */
    arena_memory_resource(arena& arena) noexcept;

private:
    void* do_allocate(size_t bytes, size_t alignment) override;
    void do_deallocate(void*, size_t, size_t) noexcept override {}
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;

    arena& arena_;
};

/**
 * @brief Deleter для std::unique_ptr, совместимый с ареной.
 *
 * Не освобождает память — она управляется ареной.
 * Позволяет использовать unique_ptr<T, arena_deleter<T>> как
 * безопасную обёртку над размещённым в арене объектом.
 *
 * @see arena_ptr
 */
template <typename T>
struct arena_deleter {
    void operator()(T*) const noexcept {}
};

/**
 * @brief Указатель на объект, размещённый в арене.
 *
 * Семантика: unique_ptr, который не вызывает delete.
 * Используется в узлах AST для безопасного хранения указателей
 * на объекты, время жизни которых управляется ареной.
 *
 * @tparam T Тип объекта
 */
template <typename T>
using arena_ptr = std::unique_ptr<T, arena_deleter<T>>;

/**
 * @brief Создает arena_ptr от типа T
 *
 * Создает arena_ptr из указателя на память,
 * выделенной в арене
 *
 * @tparam T     Произвольный тип
 * @tparam Args  Типы аргументов конструктора T
 * @param arena  Арена для размещения
 * @param args   Аргументы конструктора
 * @return       Готовый arena_ptr<T>
 */
template <typename T, typename... Args>
arena_ptr<T> make_arena(arena& arena, Args&&... args) {
    auto&& ptr = arena.allocate<T>(std::forward<Args>(args)...);
    return arena_ptr<T>(ptr);
}

}  // namespace core
