#include <gtest/gtest.h>
#include "runtime/value.hpp"
#include "core/token/token_types.hpp"
#include "core/error/error_codes.hpp"
#include <string>
#include <cstdint>

namespace tests {

TEST(value_test, default_is_void) {
    runtime::value v;
    EXPECT_EQ(v.type(), core::type::void_type());
}

TEST(value_test, int_value) {
    runtime::value v(int64_t{ 42 });
    EXPECT_EQ(v.type(), core::type::int_type());
    EXPECT_EQ(v.as_int().value(), 42);
}

TEST(value_test, double_value) {
    runtime::value v(3.14);
    EXPECT_EQ(v.type(), core::type::double_type());
    EXPECT_DOUBLE_EQ(v.as_double().value(), 3.14);
}

TEST(value_test, bool_value) {
    runtime::value v(true);
    EXPECT_EQ(v.type(), core::type::bool_type());
    EXPECT_EQ(v.as_bool().value(), true);
}

TEST(value_test, string_value) {
    runtime::value v(std::string("hello"));
    EXPECT_EQ(v.type(), core::type::string_type());
    EXPECT_EQ(v.as_string().value(), "hello");
}

TEST(value_test, int_to_string) {
    runtime::value v(int64_t{ 42 });
    EXPECT_EQ(v.to_string(), "42");
}

TEST(value_test, bool_to_string) {
    runtime::value v(true);
    EXPECT_EQ(v.to_string(), "true");
}

TEST(value_test, void_to_string) {
    runtime::value v;
    EXPECT_EQ(v.to_string(), "void");
}

TEST(value_test, int_to_double) {
    runtime::value v(int64_t{ 42 });
    EXPECT_DOUBLE_EQ(v.to_double(), 42.0);
}

TEST(value_test, double_to_int) {
    runtime::value v(3.14);
    EXPECT_EQ(v.to_int(), 3);
}

TEST(value_test, add_ints) {
    runtime::value a(int64_t{ 10 });
    runtime::value b(int64_t{ 20 });
    auto result = a.add(b);
    EXPECT_EQ(result.type(), core::type::int_type());
    EXPECT_EQ(result.as_int().value(), 30);
}

TEST(value_test, add_int_and_double) {
    runtime::value a(int64_t{ 10 });
    runtime::value b(3.5);
    auto result = a.add(b);
    EXPECT_EQ(result.type(), core::type::double_type());
    EXPECT_DOUBLE_EQ(result.as_double().value(), 13.5);
}

TEST(value_test, sub_ints) {
    runtime::value a(int64_t{ 30 });
    runtime::value b(int64_t{ 10 });
    auto result = a.sub(b);
    EXPECT_EQ(result.as_int().value(), 20);
}

TEST(value_test, mul_ints) {
    runtime::value a(int64_t{ 6 });
    runtime::value b(int64_t{ 7 });
    auto result = a.mul(b);
    EXPECT_EQ(result.as_int().value(), 42);
}

TEST(value_test, div_ints) {
    runtime::value a(int64_t{ 20 });
    runtime::value b(int64_t{ 4 });
    auto result = a.div(b);
    EXPECT_EQ(result.as_int().value(), 5);
}

TEST(value_test, div_by_zero) {
    runtime::value a(int64_t{ 1 });
    runtime::value b(int64_t{ 0 });
    EXPECT_THROW(a.div(b), core::interpret_error);
}

TEST(value_test, mod_ints) {
    runtime::value a(int64_t{ 17 });
    runtime::value b(int64_t{ 5 });
    auto result = a.mod(b);
    EXPECT_EQ(result.as_int().value(), 2);
}

TEST(value_test, mod_by_zero) {
    runtime::value a(int64_t{ 1 });
    runtime::value b(int64_t{ 0 });
    EXPECT_THROW(a.mod(b), core::interpret_error);
}

TEST(value_test, eq_ints) {
    runtime::value a(int64_t{ 42 });
    runtime::value b(int64_t{ 42 });
    runtime::value c(int64_t{ 0 });
    EXPECT_TRUE(a.eq(b).as_bool().value());
    EXPECT_FALSE(a.eq(c).as_bool().value());
}

TEST(value_test, lt_ints) {
    runtime::value a(int64_t{ 10 });
    runtime::value b(int64_t{ 20 });
    EXPECT_TRUE(a.lt(b).as_bool().value());
    EXPECT_FALSE(b.lt(a).as_bool().value());
}

TEST(value_test, and_op) {
    runtime::value t(true);
    runtime::value f(false);
    EXPECT_TRUE(t.and_op(t).as_bool().value());
    EXPECT_FALSE(t.and_op(f).as_bool().value());
    EXPECT_FALSE(f.and_op(t).as_bool().value());
}

TEST(value_test, or_op) {
    runtime::value t(true);
    runtime::value f(false);
    EXPECT_TRUE(t.or_op(t).as_bool().value());
    EXPECT_TRUE(t.or_op(f).as_bool().value());
    EXPECT_FALSE(f.or_op(f).as_bool().value());
}

TEST(value_test, not_op) {
    runtime::value t(true);
    runtime::value f(false);
    EXPECT_FALSE(t.not_op().as_bool().value());
    EXPECT_TRUE(f.not_op().as_bool().value());
}

TEST(value_test, invalid_add) {
    runtime::value a(true);
    runtime::value b(int64_t{ 1 });
    EXPECT_THROW(a.add(b), core::interpret_error);
}

} // namespace tests