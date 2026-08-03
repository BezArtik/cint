// core/value/operations.cpp

#include "core/value/operations.hpp"

#include "core/error/error_codes.hpp"

#include <functional>

namespace core::ops {

namespace {

template <typename Op>
value arithmetic_op(const value& a, const value& b, Op&& op) {
    if (a.is_int() && b.is_int()) return value{op(*a.as<value::int_t>(), *b.as<value::int_t>())};
    return op(a.to_double(), b.to_double());
}

}  // namespace

value add(const value& a, const value& b) {
    return arithmetic_op(a, b, std::plus<>{});
}
value sub(const value& a, const value& b) {
    return arithmetic_op(a, b, std::minus<>{});
}
value mul(const value& a, const value& b) {
    return arithmetic_op(a, b, std::multiplies<>{});
}

value div(const value& a, const value& b) {
    return arithmetic_op(a, b, [](auto x, auto y) {
        if (y == 0) throw core::value_error{error_code::division_by_zero};
        return x / y;
    });
}

value mod(const value& a, const value& b) {
    auto&& li = a.to_int();
    auto&& ri = b.to_int();
    if (ri == 0) throw core::value_error{error_code::modulo_by_zero};
    return li % ri;
}

value unary_minus(const value& a) {
    if (a.is_int()) return -a.to_int();
    return -a.to_double();
}

value eq(const value& a, const value& b) {
    if (a.is_int() && b.is_int()) return a.to_int() == b.to_int();
    if (a.is_double() && b.is_double()) return a.to_double() == b.to_double();
    if (a.is_bool() && b.is_bool()) return a.to_bool() == b.to_bool();
    if (a.is_string() && b.is_string()) return a.to_string() == b.to_string();

    if ((a.is_int() || a.is_double()) && (b.is_int() || b.is_double())) return a.to_double() == b.to_double();

    return false;
}

value neq(const value& a, const value& b) {
    return not_op(eq(a, b));
}

value lt(const value& a, const value& b) {
    return arithmetic_op(a, b, std::less<>{});
}

value le(const value& a, const value& b) {
    return arithmetic_op(a, b, std::less_equal<>{});
}
value gt(const value& a, const value& b) {
    return arithmetic_op(a, b, std::greater<>{});
}
value ge(const value& a, const value& b) {
    return arithmetic_op(a, b, std::greater_equal<>{});
}

value not_op(const value& a) {
    return !a.to_bool();
}

value bit_and(const value& a, const value& b) {
    return a.to_int() & b.to_int();
}
value bit_or(const value& a, const value& b) {
    return a.to_int() | b.to_int();
}
value bit_xor(const value& a, const value& b) {
    return a.to_int() ^ b.to_int();
}
value bit_not(const value& a) {
    return ~a.to_int();
}
value shl(const value& a, const value& b) {
    return a.to_int() << b.to_int();
}
value shr(const value& a, const value& b) {
    return a.to_int() >> b.to_int();
}

}  // namespace core::ops
