// core/utils/overloaded.hpp
#pragma once

namespace cint::core {

template <typename... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};

}  // namespace cint::core
