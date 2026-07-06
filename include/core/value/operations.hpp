// value/operations.hpp

#pragma once
#include "core/value/value.hpp"

namespace core::ops {

value add(const value& a, const value& b);
value sub(const value& a, const value& b);
value mul(const value& a, const value& b);
value div(const value& a, const value& b);
value mod(const value& a, const value& b);
value eq(const value& a, const value& b);
value neq(const value& a, const value& b);
value lt(const value& a, const value& b);
value le(const value& a, const value& b);
value gt(const value& a, const value& b);
value ge(const value& a, const value& b);
value and_op(const value& a, const value& b);
value or_op(const value& a, const value& b);
value not_op(const value& a);

}  // namespace core::ops
