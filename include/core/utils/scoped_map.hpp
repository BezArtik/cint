/**
 * @file include/core/utils/scoped_map.hpp
 * @brief Словарь с поддержкой областей видимости.
 * @ingroup CoreUtils
 */

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory_resource>
#include <string_view>
#include <vector>

namespace core {

/**
 * @brief Словарь со стеком областей видимости.
 * @ingroup CoreUtils
 *
 * Реализует семантику вложенных блоков: переменные, объявленные
 * во внутреннем блоке, затеняют переменные с тем же именем из
 * внешних блоков. При выходе из блока его переменные удаляются.
 *
 * Структура данных: **линейный вектор с маркерами глубины**.
 *
 * Операции и их сложность:
 * - push():    O(1) — инкремент счётчика глубины
 * - pop():     O(k) — удаление k записей текущего уровня
 * - define():  O(1) — добавление в конец вектора
 * - get():     O(n) — линейный поиск от конца к началу
 * - contains_in_current_scope(): O(k) — поиск только в текущем уровне
 *
 * @tparam T Тип хранимых значений.
 *
 */
template <typename T>
class scoped_map {
public:
    scoped_map() = default;

    /**
     * @brief Открывает новую область видимости.
     *
     * Увеличивает счётчик глубины. Последующие define() будут
     * ассоциированы с этой областью. Области могут быть вложенными.
     */
    void push() noexcept { ++current_depth_; }

    /**
     * @brief Закрывает текущую область видимости.
     *
     * Удаляет все записи с текущей глубиной (pop с конца вектора).
     * Переменные внешних областей снова становятся видимыми.
     *
     */
    void pop() noexcept {
        while (!entries_.empty() && entries_.back().scope_depth_ == current_depth_) entries_.pop_back();
        --current_depth_;
    }

    /**
     * @brief Объявляет переменную в текущей области видимости.
     *
     * @param name  Имя переменной
     * @param value Значение
     *
     * @note Не проверяет повторные объявления — это задача семантического анализатора.
     */
    void define(std::string_view name, T value) { entries_.emplace_back(name, std::move(value), current_depth_); }

    /**
     * @brief Ищет переменную по имени (с учётом затенения).
     *
     * Просматривает записи от самой глубокой области к глобальной,
     * возвращая первое совпадение. Таким образом, переменные
     * внутренних блоков затеняют переменные внешних.
     *
     * @param name Имя переменной
     * @return Указатель на значение или nullptr, если переменная не найдена.
     */
    T* get(std::string_view name) noexcept { return get_impl(*this, name); }
    const T* get(std::string_view name) const noexcept { return get_impl(*this, name); }

    /**
     * @brief Проверяет, объявлена ли переменная в текущей области.
     *
     * Используется для обнаружения повторных объявлений.
     * Просматривает только записи текущей глубины.
     *
     * @param name Имя переменной
     * @return true, если переменная уже объявлена в текущем блоке.
     */
    bool contains_in_current_scope(std::string_view name) const noexcept {
        for (auto it = entries_.rbegin(), endit = entries_.rend(); it != endit; ++it) {
            if (it->scope_depth_ < current_depth_) break;
            if (it->name_ == name) return true;
        }
        return false;
    }

private:
    /**
     * @brief Запись словаря: имя, значение, глубина области видимости.
     *
     * Глубина 0 — глобальная область. Каждый push() увеличивает счётчик.
     */
    struct entry {
        std::string_view name_;
        T value_;
        size_t scope_depth_;
    };

    std::array<std::byte, 4096> buffer_;                                      ///< Статический буфер для оптимизации
    std::pmr::monotonic_buffer_resource mr_{buffer_.data(), buffer_.size()};  ///< mr для передачи в pmr::vector
    std::pmr::vector<entry> entries_{&mr_};  ///< Все записи (от глобальных к локальным)
    size_t current_depth_ = 0;               ///< Текущая глубина (0 — глобальная)

    template <typename Self>
    static auto* get_impl(Self& self, std::string_view name) noexcept {
        auto&& it = std::find_if(self.entries_.rbegin(), self.entries_.rend(),
                                 [&](auto&& entry) { return entry.name_ == name; });
        return it != self.entries_.rend() ? &it->value_ : nullptr;
    }
};

/**
 * @brief RAII-guard для автоматического управления областями видимости.
 * @ingroup CoreUtils
 *
 * Открывает область видимости в конструкторе и закрывает в деструкторе.
 * Гарантирует корректный возврат к предыдущей области даже при исключениях.
 */
template <typename T>
class scope_guard {
public:
    scope_guard(scoped_map<T>& map) : map_(map) { map_.push(); }
    ~scope_guard() { map_.pop(); }

    scope_guard(const scope_guard&) = delete;
    scope_guard& operator=(const scope_guard&) = delete;

private:
    scoped_map<T>& map_;
};

}  // namespace core
