// core/value/operations.cpp

#include "core/value/operations.hpp"

#include "core/error/error_codes.hpp"

#include <functional>

namespace core::ops {

namespace {

template <typename Op>
value arithmetic_op(const value& a, const value& b, Op&& op) {
    if (a.is_int() && b.is_int()) return value{op(*a.as<value::int_t>(), *b.as<value::int_t>())};
    return value{op(a.to_double(), b.to_double())};
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
        if (y == 0) throw core::interpret_error{error_code::division_by_zero};
        return x / y;
    });
}

value mod(const value& a, const value& b) {
    auto li = *a.as<value::int_t>();
    auto ri = *b.as<value::int_t>();
    if (ri == 0) throw core::interpret_error{error_code::modulo_by_zero};
    return value{li % ri};
}

value eq(const value& a, const value& b) {
    if (a.is_int() && b.is_int()) return value{*a.as<value::int_t>() == *b.as<value::int_t>()};
    if (a.is_double() && b.is_double()) return value{*a.as<value::double_t>() == *b.as<value::double_t>()};
    if (a.is_bool() && b.is_bool()) return value{*a.as<value::bool_t>() == *b.as<value::bool_t>()};
    if (a.is_string() && b.is_string()) return value{a.as<value::string_t>()->get() == b.as<value::string_t>()->get()};

    if ((a.is_int() || a.is_double()) && (b.is_int() || b.is_double())) return value{a.to_double() == b.to_double()};

    return value{false};
}

value neq(const value& a, const value& b) {
    return not_op(eq(a, b));
}

value lt(const value& a, const value& b) {
    if (a.is_int() && b.is_int()) return value{*a.as<value::int_t>() < *b.as<value::int_t>()};
    return value{a.to_double() < b.to_double()};
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
    return value{a.to_bool() && b.to_bool()};
}
value or_op(const value& a, const value& b) {
    return value{a.to_bool() || b.to_bool()};
}
value not_op(const value& a) {
    return value{!a.to_bool()};
}

value bit_and(const value& a, const value& b) {
    return value{*a.as<value::int_t>() & *b.as<value::int_t>()};
}
value bit_or(const value& a, const value& b) {
    return value{*a.as<value::int_t>() | *b.as<value::int_t>()};
}
value bit_xor(const value& a, const value& b) {
    return value{*a.as<value::int_t>() ^ *b.as<value::int_t>()};
}
value bit_not(const value& a) {
    return value{~*a.as<value::int_t>()};
}
value shl(const value& a, const value& b) {
    return value{*a.as<value::int_t>() << *b.as<value::int_t>()};
}
value shr(const value& a, const value& b) {
    return value{*a.as<value::int_t>() >> *b.as<value::int_t>()};
}

}  // namespace core::ops
