// core/value/operations.cpp

#include "core/value/operations.hpp"

#include "core/error/error_codes.hpp"
#include "core/token/token_types.hpp"

#include <cassert>
#include <functional>

namespace core::ops {

using err = core::error_code;

namespace {

template <typename Op>
value arithmetic_op(const value& a, const value& b, Op&& op) {
    if (a.type().is_int() && b.type().is_int()) return value(op(a.to_int(), b.to_int()));
    return value(op(a.to_double(), b.to_double()));
}

}  // anonymous namespace

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
        if (y == 0) throw core::interpret_error{err::division_by_zero};
        return x / y;
    });
}

value mod(const value& a, const value& b) {
    auto li = a.as_int();
    auto ri = b.as_int();
    if (*ri == 0) throw core::interpret_error{err::modulo_by_zero};
    return value(*li % *ri);
}

value eq(const value& a, const value& b) {
    auto lt = a.type();
    auto rt = b.type();
    if (lt != rt) {
        if (lt.is_numeric() && rt.is_numeric()) return value(a.to_double() == b.to_double());
        return value(false);
    }
    if (lt.is_int()) return value(a.to_int() == b.to_int());
    if (lt.is_double()) return value(a.to_double() == b.to_double());
    if (lt.is_bool()) return value(a.to_bool() == b.to_bool());
    if (lt.is_string()) return value(a.to_string() == b.to_string());
    return value(false);
}

value neq(const value& a, const value& b) {
    return not_op(eq(a, b));
}

value lt(const value& a, const value& b) {
    if (a.type().is_int() && b.type().is_int()) return value(a.to_int() < b.to_int());
    return value(a.to_double() < b.to_double());
}

value le(const value& a, const value& b) {
    return or_op(lt(a, b), eq(a, b));
}

value gt(const value& a, const value& b) {
    return not_op(le(a, b));
}

value ge(const value& a, const value& b) {
    return not_op(lt(a, b));
}

value and_op(const value& a, const value& b) {
    return value(a.to_bool() && b.to_bool());
}

value or_op(const value& a, const value& b) {
    return value(a.to_bool() || b.to_bool());
}

value not_op(const value& a) {
    return value(!a.to_bool());
}

value bit_and(const value& a, const value& b) {
    return value(a.to_int() & b.to_int());
}

value bit_or(const value& a, const value& b) {
    return value(a.to_int() | b.to_int());
}

value bit_xor(const value& a, const value& b) {
    return value(a.to_int() ^ b.to_int());
}

value bit_not(const value& a) {
    return value(~a.to_int());
}

value shl(const value& a, const value& b) {
    return value(a.to_int() << b.to_int());
}

value shr(const value& a, const value& b) {
    return value(a.to_int() >> b.to_int());
}

}  // namespace core::ops
