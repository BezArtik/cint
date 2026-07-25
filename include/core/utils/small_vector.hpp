/**
 * @file include/core/utils/small_vector.hpp
 * @brief Вектор с буфером на стеке для малых размеров (small vector optimization).
 * @ingroup CoreUtils
 */

#pragma once

#include <cstddef>
#include <vector>

namespace core {

/**
 * @brief Вектор, хранящий до N элементов на стеке без динамической памяти.
 * @ingroup CoreUtils
 *
 * Реализует **small vector optimization** (SVO): для размера ≤ N элементы
 * размещаются во встроенном буфере на стеке. При превышении N переключается
 * на динамическое выделение через стандартный аллокатор.
 *
 * Преимущества:
 * - **Нет аллокаций** для типичных сценариев (список параметров, аргументов, размерностей массива)
 * - **Локальность данных**: элементы на стеке — в кеше
 * - **Совместимость**: повторяет интерфейс std::vector
 *
 * Ограничения:
 * - **Не копируется и не перемещается** (буфер на стеке)
 * - **Только для локального использования** (не как поле долгоживущего объекта)
 *
 * Реализация:
 * - stack_allocator перенаправляет выделения
 *   до N элементов во встроенный буфер
 * - При превышении — делегирует ::operator new
 * - При освобождении проверяет, был ли использован буфер
 *
 * @tparam T Тип элементов
 * @tparam N Размер встроенного буфера (в элементах)
 */
template <typename T, size_t N>
class small_vector {
    /**
     * @brief Аллокатор, использующий буфер на стеке.
     *
     * Принимает указатель на внешний буфер (buffer_) и флаг занятости (used_).
     * Первый вызов allocate() для размера ≤ N использует буфер,
     * остальные — стандартное выделение.
     */
    template <typename U>
    struct stack_allocator {
        using value_type = U;

        /**
         * @brief Конструктор.
         * @param buffer Указатель на встроенный буфер
         * @param used   Флаг занятости буфера
         */
        stack_allocator(std::byte* buffer, bool* used) noexcept : buffer_(buffer), used_(used) {}

        /// Конструктор копирования для совместимости с std::vector (rebind).
        template <typename V>
        stack_allocator(const stack_allocator<V>& other) noexcept : buffer_(other.buffer_), used_(other.used_) {}

        /**
         * @brief Выделяет память для n элементов.
         *
         * При первом вызове и n ≤ N — возвращает встроенный буфер.
         * Иначе — делегирует ::operator new.
         *
         * @param n Количество элементов
         * @return Указатель на выделенную память.
         */
        U* allocate(size_t n) {
            if constexpr (std::is_same_v<U, T>) {
                if (!*used_ && n <= N) {
                    *used_ = true;
                    return reinterpret_cast<U*>(buffer_);
                }
            }
            return static_cast<U*>(::operator new(n * sizeof(U)));
        }

        /**
         * @brief Освобождает память.
         *
         * Если был использован встроенный буфер — сбрасывает флаг.
         * Иначе — вызывает ::operator delete.
         */
        void deallocate(U* p, size_t) noexcept {
            if constexpr (std::is_same_v<U, T>) {
                if (p == reinterpret_cast<U*>(buffer_)) {
                    *used_ = false;
                    return;
                }
            }
            ::operator delete(p);
        }

        /// Сравнение аллокаторов (равны, если указывают на один буфер).
        template <typename V>
        bool operator==(const stack_allocator<V>& other) const noexcept {
            return buffer_ == other.buffer_;
        }

        template <typename V>
        bool operator!=(const stack_allocator<V>& other) const noexcept {
            return !(*this == other);
        }

        std::byte* buffer_;  ///< Указатель на встроенный буфер
        bool* used_;         ///< Флаг занятости буфера
    };

public:
    /// @name Типы (совместимость с std::vector)
    /// @{
    using container_type = std::vector<T, stack_allocator<T>>;
    using value_type = typename container_type::value_type;
    using pointer = typename container_type::pointer;
    using const_pointer = typename container_type::const_pointer;
    using reference = typename container_type::reference;
    using const_reference = typename container_type::const_reference;
    using iterator = typename container_type::iterator;
    using const_iterator = typename container_type::const_iterator;
    using reverse_iterator = typename container_type::reverse_iterator;
    using const_reverse_iterator = typename container_type::const_reverse_iterator;
    using size_type = typename container_type::size_type;
    /// @}

    /// Создаёт вектор с резервированием N элементов (в стековом буфере).
    small_vector() : vec_(stack_allocator<value_type>(buffer_, &used_)) { vec_.reserve(N); }

    /// @name Копирование и перемещение запрещены (буфер на стеке)
    /// @{
    small_vector(const small_vector&) = delete;
    small_vector& operator=(const small_vector&) = delete;
    small_vector(small_vector&&) = delete;
    small_vector& operator=(small_vector&&) = delete;
    /// @}

    /// @name Интерфейс std::vector (частичный)
    /// @{

    void push_back(const_reference value) { vec_.push_back(value); }
    void push_back(value_type&& value) { vec_.push_back(std::move(value)); }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        vec_.emplace_back(std::forward<Args>(args)...);
    }

    void reserve(size_type n) { vec_.reserve(n); }

    reference operator[](size_type i) noexcept { return vec_[i]; }
    const_reference operator[](size_type i) const noexcept { return vec_[i]; }

    pointer data() noexcept { return vec_.data(); }
    const_pointer data() const noexcept { return vec_.data(); }

    iterator begin() noexcept { return vec_.begin(); }
    iterator end() noexcept { return vec_.end(); }
    const_iterator begin() const noexcept { return vec_.begin(); }
    const_iterator end() const noexcept { return vec_.end(); }
    reverse_iterator rbegin() noexcept { return vec_.rbegin(); }
    reverse_iterator rend() noexcept { return vec_.rend(); }
    const_reverse_iterator rbegin() const noexcept { return vec_.rbegin(); }
    const_reverse_iterator rend() const noexcept { return vec_.rend(); }

    /// @}

private:
    /// Встроенный буфер на стеке (выровнен как value_type).
    alignas(value_type) std::byte buffer_[N * sizeof(value_type)];

    /// Флаг: используется ли сейчас встроенный буфер.
    bool used_ = false;

    /// Реальный вектор с кастомным аллокатором.
    container_type vec_;
};

}  // namespace core
