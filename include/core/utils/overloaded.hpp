/**
 * @file include/core/utils/overloaded.hpp
 * @brief Утилиты для работы с std::variant и std::visit.
 * @ingroup CoreUtils
 */

#pragma once

namespace core {

/**
 * @brief Вспомогательный шаблон для создания visitor'а "на лету".
 * @ingroup CoreUtils
 *
 * Позволяет передать несколько лямбд в std::visit без явного
 * определения класса-visitor'а. Основан на множественном наследовании
 * от переданных типов лямбд и использовании их operator().
 *
 *
 * @tparam Ts Типы лямбд-обработчиков (по одному на каждый тип в variant)
 */
template <typename... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

}  // namespace core
