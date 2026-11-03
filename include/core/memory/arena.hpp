/**
 * @file include/core/memory/arena.hpp
 * @brief Арена-аллокатор и адаптер для std::pmr.
 * @ingroup Core
 *
 * @defgroup CoreUtils Утилиты
 * @brief Вспомогательные классы и структуры данных ядра интерпретатора.
 */

#pragma once

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace cint::core {

/**
 * @brief Арена-аллокатор с поддержкой деструкторов.
 * @ingroup CoreUtils
 *
 * Реализует стратегию **region-based memory management**:
 * - Память выделяется крупными блоками (16 КБ по умолчанию)
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
class [[nodiscard]] arena {
    /// Размер блока по умолчанию: 16 КБ.
    static constexpr std::size_t BLOCK_SIZE = 16 * 1024;

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
    ~arena() { reset(); }

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
        const auto ptr = allocate_raw(sizeof(T), alignof(T));
        const auto obj = std::construct_at<T>(static_cast<T*>(ptr), std::forward<Args>(args)...);

        if constexpr (!std::is_trivially_destructible_v<T>)
            destructors_.emplace_back(obj, [](void* p) { std::destroy_at<T>(static_cast<T*>(p)); });

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
    void* allocate_raw(std::size_t size, std::size_t alignment);

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
    /**
     * @brief Запись реестра деструкторов.
     */
    struct destructor_entry {
        void* obj_;               /// указатель на объект
        void (*destroy_)(void*);  /// указатель на функцию (деструктор)
    };

    // Данные
    std::vector<std::unique_ptr<std::byte[]>> blocks_;  ///< Выделенные блоки памяти
    std::vector<destructor_entry> destructors_;         ///< Зарегистрированные деструкторы
    std::byte* current_ = nullptr;                      ///< Текущая позиция в активном блоке
    std::byte* end_ = nullptr;                          ///< Конец активного блока
};

template <typename T>
class [[nodiscard]] arena_allocator {
public:
    using value_type = T;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;

    arena_allocator() noexcept = default;
    arena_allocator(arena& a) noexcept : arena_{&a} {}

    template <typename U>
    arena_allocator(const arena_allocator<U>& other) noexcept : arena_{other.arena_} {}

    T* allocate(std::size_t n) {
        if (!arena_) throw std::bad_alloc{};
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T)) throw std::bad_array_new_length{};
        return static_cast<T*>(arena_->allocate_raw(n * sizeof(T), alignof(T)));
    }

    void deallocate(T*, std::size_t) noexcept {}

    template <typename U>
    friend class arena_allocator;
    template <typename U>
    friend bool operator==(const arena_allocator<U>&, const arena_allocator<U>&) noexcept;

private:
    arena* arena_ = nullptr;
};

template <typename T, typename U>
[[nodiscard]] bool operator==(const arena_allocator<T>& a, const arena_allocator<U>& b) noexcept {
    return a.arena_ == b.arena_;
}

template <typename T, typename U>
[[nodiscard]] bool operator!=(const arena_allocator<T>& a, const arena_allocator<U>& b) noexcept {
    return !(a == b);
}

/*
 * @brief Псевдоним для контейнеров, использующего арену.
 *
 * @tparam T Тип объекта
 */
template <typename T>
using arena_vector = std::vector<T, arena_allocator<T>>;

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
[[nodiscard]] auto make_arena(arena& arena, Args&&... args) {
    return arena_ptr<T>{arena.allocate<T>(std::forward<Args>(args)...)};
}

}  // namespace cint::core
