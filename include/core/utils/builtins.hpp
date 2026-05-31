// core/utils/builtins.hpp


#pragma once
#include "core/token/token.hpp"
#include "core/token/token_types.hpp"
#include <string>
#include <vector>
#include <functional>


namespace runtime { class value; class environment; }

namespace core {

struct builtin_def {
    std::string name_;
    std::vector<std::vector<type>> overloads_; 
    type return_type_;
    std::function<runtime::value(const std::vector<runtime::value>&)> impl_;
};

std::vector<builtin_def> builtins();

} // namespace core
