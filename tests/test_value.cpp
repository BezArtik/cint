// test_value.cpp

#include "core/error/error_codes.hpp"
#include "core/token/token_types.hpp"
#include "core/value/operations.hpp"
#include "core/value/value.hpp"

#include <gtest/gtest.h>
#include <string>

namespace tests {

using t = core::type;
using v = core::value;
namespace op = core::ops;

TEST(value_test, default_is_void) {
    v val;
    EXPECT_TRUE(val.is_void());
}

TEST(value_test, int_value) {
    v val(v::int_t{42});
    EXPECT_TRUE(val.is_int());
    EXPECT_EQ(*val.as<v::int_t>(), 42);
}

TEST(value_test, double_value) {
    v val(3.14);
    EXPECT_TRUE(val.is_double());
    EXPECT_DOUBLE_EQ(*val.as<v::double_t>(), 3.14);
}

TEST(value_test, bool_value) {
    v val(true);
    EXPECT_TRUE(val.is_bool());
    EXPECT_EQ(*val.as<v::bool_t>(), true);
}

TEST(value_test, string_value) {
    v val(std::string("hello"));
    EXPECT_TRUE(val.is_string());
    EXPECT_EQ(val.as<v::string_t>()->get(), "hello");
}

TEST(value_test, int_to_string) {
    v val(v::int_t{42});
    EXPECT_EQ(val.to_string(), "42");
}

TEST(value_test, bool_to_string) {
    v val(true);
    EXPECT_EQ(val.to_string(), "true");
}

TEST(value_test, void_to_string) {
    v val;
    EXPECT_EQ(val.to_string(), "void");
}

TEST(value_test, int_to_double) {
    v val(v::int_t{42});
    EXPECT_DOUBLE_EQ(val.to_double(), 42.0);
}

TEST(value_test, double_to_int) {
    v val(3.14);
    EXPECT_EQ(val.to_int(), 3);
}

TEST(value_test, add_ints) {
    v a(v::int_t{10});
    v b(v::int_t{20});
    auto result = op::add(a, b);
    EXPECT_TRUE(result.is_int());
    EXPECT_EQ(*result.as<v::int_t>(), 30);
}

TEST(value_test, add_int_and_double) {
    v a(v::int_t{10});
    v b(3.5);
    auto result = op::add(a, b);
    EXPECT_TRUE(result.is_double());
    EXPECT_DOUBLE_EQ(*result.as<v::double_t>(), 13.5);
}

TEST(value_test, sub_ints) {
    v a(v::int_t{30});
    v b(v::int_t{10});
    auto result = op::sub(a, b);
    EXPECT_EQ(*result.as<v::int_t>(), 20);
}

TEST(value_test, mul_ints) {
    v a(v::int_t{6});
    v b(v::int_t{7});
    auto result = op::mul(a, b);
    EXPECT_EQ(*result.as<v::int_t>(), 42);
}

TEST(value_test, div_ints) {
    v a(v::int_t{20});
    v b(v::int_t{4});
    auto result = op::div(a, b);
    EXPECT_EQ(*result.as<v::int_t>(), 5);
}

TEST(value_test, div_by_zero) {
    v a(v::int_t{1});
    v b(v::int_t{0});
    EXPECT_THROW(op::div(a, b), core::interpret_error);
}

TEST(value_test, mod_ints) {
    v a(v::int_t{17});
    v b(v::int_t{5});
    auto result = op::mod(a, b);
    EXPECT_EQ(*result.as<v::int_t>(), 2);
}

TEST(value_test, mod_by_zero) {
    v a(v::int_t{1});
    v b(v::int_t{0});
    EXPECT_THROW(op::mod(a, b), core::interpret_error);
}

TEST(value_test, eq_ints) {
    v a(v::int_t{42});
    v b(v::int_t{42});
    v c(v::int_t{0});
    EXPECT_TRUE(*op::eq(a, b).as<v::bool_t>());
    EXPECT_FALSE(*op::eq(a, c).as<v::bool_t>());
}

TEST(value_test, lt_ints) {
    v a(v::int_t{10});
    v b(v::int_t{20});
    EXPECT_TRUE(*op::lt(a, b).as<v::bool_t>());
    EXPECT_FALSE(*op::lt(b, a).as<v::bool_t>());
}

TEST(value_test, and_op) {
    v t(true);
    v f(false);
    EXPECT_TRUE(*op::and_op(t, t).as<v::bool_t>());
    EXPECT_FALSE(*op::and_op(t, f).as<v::bool_t>());
    EXPECT_FALSE(*op::and_op(f, t).as<v::bool_t>());
}

TEST(value_test, or_op) {
    v t(true);
    v f(false);
    EXPECT_TRUE(*op::or_op(t, t).as<v::bool_t>());
    EXPECT_TRUE(*op::or_op(t, f).as<v::bool_t>());
    EXPECT_FALSE(*op::or_op(f, f).as<v::bool_t>());
}

TEST(value_test, not_op) {
    v t(true);
    v f(false);
    EXPECT_FALSE(*op::not_op(t).as<v::bool_t>());
    EXPECT_TRUE(*op::not_op(f).as<v::bool_t>());
}

TEST(value_test, invalid_add) {
    v a(true);
    v b(v::int_t{1});
    EXPECT_THROW(op::add(a, b), core::interpret_error);
}

}  // namespace tests
