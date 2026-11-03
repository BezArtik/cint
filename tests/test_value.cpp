// tests/test_value.cpp

#include "core/error/error_codes.hpp"
#include "core/value/operations.hpp"
#include "core/value/value.hpp"

#include <gtest/gtest.h>
#include <string>

namespace cint::tests {

using t = core::type;
using v = core::value;

TEST(value_test, default_is_void) {
    auto val = v{};
    EXPECT_TRUE(val.is_void());
}

TEST(value_test, int_value) {
    auto val = v{v::int_t{42}};
    EXPECT_EQ(val.to_int(), 42);
}

TEST(value_test, double_value) {
    auto val = v{3.14};
    EXPECT_DOUBLE_EQ(val.to_double(), 3.14);
}

TEST(value_test, bool_value) {
    auto val = v{true};
    EXPECT_EQ(val.to_bool(), true);
}

TEST(value_test, string_value) {
    auto val = v{std::string{"hello"}};
    EXPECT_EQ(val.to_string(), "hello");
}

TEST(value_test, int_to_string) {
    auto val = v{v::int_t{42}};
    EXPECT_EQ(val.to_string(), "42");
}

TEST(value_test, bool_to_string) {
    auto val = v{true};
    EXPECT_EQ(val.to_string(), "true");
}

TEST(value_test, void_to_string) {
    auto val = v{};
    EXPECT_EQ(val.to_string(), "void");
}

TEST(value_test, int_to_double) {
    auto val = v{v::int_t{42}};
    EXPECT_DOUBLE_EQ(val.to_double(), 42.0);
}

TEST(value_test, double_to_int) {
    auto val = v{3.14};
    EXPECT_EQ(val.to_int(), 3);
}

TEST(value_test, add_ints) {
    auto a = v{v::int_t{10}};
    auto b = v{v::int_t{20}};
    EXPECT_EQ(core::add(a, b).to_int(), 30);
}

TEST(value_test, add_int_and_double) {
    auto a = v{v::int_t{10}};
    auto b = v{3.5};
    EXPECT_DOUBLE_EQ(core::add(a, b).to_double(), 13.5);
}

TEST(value_test, sub_ints) {
    auto a = v{v::int_t{30}};
    auto b = v{v::int_t{10}};
    EXPECT_EQ(core::sub(a, b).to_int(), 20);
}

TEST(value_test, mul_ints) {
    auto a = v{v::int_t{6}};
    auto b = v{v::int_t{7}};
    EXPECT_EQ(core::mul(a, b).to_int(), 42);
}

TEST(value_test, div_ints) {
    auto a = v{v::int_t{20}};
    auto b = v{v::int_t{4}};
    EXPECT_EQ(core::div(a, b).to_int(), 5);
}

TEST(value_test, div_by_zero) {
    auto a = v{v::int_t{1}};
    auto b = v{v::int_t{0}};
    EXPECT_THROW(std::ignore = core::div(a, b), core::value_error);
}

TEST(value_test, mod_ints) {
    auto a = v{v::int_t{17}};
    auto b = v{v::int_t{5}};
    EXPECT_EQ(core::mod(a, b).to_int(), 2);
}

TEST(value_test, mod_by_zero) {
    auto a = v{v::int_t{1}};
    auto b = v{v::int_t{0}};
    EXPECT_THROW(std::ignore = core::mod(a, b), core::value_error);
}

TEST(value_test, eq_ints) {
    auto a = v{v::int_t{42}};
    auto b = v{v::int_t{42}};
    auto c = v{v::int_t{0}};
    EXPECT_TRUE(core::eq(a, b).to_bool());
    EXPECT_FALSE(core::eq(a, c).to_bool());
}

TEST(value_test, lt_ints) {
    auto a = v{v::int_t{10}};
    auto b = v{v::int_t{20}};
    EXPECT_TRUE(core::lt(a, b).to_bool());
    EXPECT_FALSE(core::lt(b, a).to_bool());
}

TEST(value_test, not_op) {
    auto t = v{true};
    auto f = v{false};
    EXPECT_FALSE(core::not_op(t).to_bool());
    EXPECT_TRUE(core::not_op(f).to_bool());
}

TEST(value_test, unary_minus_int) {
    auto a = v{v::int_t{42}};
    EXPECT_EQ(core::unary_minus(a).to_int(), -42);
}

TEST(value_test, bit_and) {
    auto a = v{v::int_t{0b1100}};
    auto b = v{v::int_t{0b1010}};
    EXPECT_EQ(core::bit_and(a, b).to_int(), 0b1000);
}

TEST(value_test, bit_or) {
    auto a = v{v::int_t{0b1001}};
    auto b = v{v::int_t{0b0110}};
    EXPECT_EQ(core::bit_or(a, b).to_int(), 0b1111);
}

TEST(value_test, bit_xor) {
    auto a = v{v::int_t{0b1011}};
    auto b = v{v::int_t{0b0110}};
    EXPECT_EQ(core::bit_xor(a, b).to_int(), 0b1101);
}

TEST(value_test, bit_not) {
    auto a = v{v::int_t{10}};
    EXPECT_EQ(core::bit_not(a).to_int(), -11);
}

TEST(value_test, shl) {
    auto a = v{v::int_t{0b0001}};
    auto b = v{v::int_t{2}};
    EXPECT_EQ(core::shl(a, b).to_int(), 0b0100);
}

TEST(value_test, shr) {
    auto a = v{v::int_t{0b1000}};
    auto b = v{v::int_t{3}};
    EXPECT_EQ(core::shr(a, b).to_int(), 0b0001);
}

TEST(value_test, invalid_add) {
    auto a = v{true};
    auto b = v{v::int_t{1}};
    EXPECT_THROW(std::ignore = core::add(a, b), core::value_error);
}

TEST(value_test, struct_type_int_fields) {
    auto&& st = t::struct_type("Point", {{"x", t::int_type()}, {"y", t::int_type()}});
    EXPECT_TRUE(st.is_struct());
    EXPECT_EQ(st.struct_name(), "Point");
    EXPECT_EQ(st.struct_fields().size(), 2);
    EXPECT_EQ(st.struct_fields()[0].first, "x");
    EXPECT_TRUE(st.struct_fields()[0].second.is_int());
    EXPECT_EQ(st.struct_fields()[1].first, "y");
    EXPECT_TRUE(st.struct_fields()[1].second.is_int());
}

TEST(value_test, struct_field_index) {
    auto&& st = t::struct_type("Point", {{"x", t::int_type()}, {"y", t::int_type()}});
    EXPECT_EQ(st.field_index("x").value(), 0);
    EXPECT_EQ(st.field_index("y").value(), 1);
    EXPECT_FALSE(st.field_index("z").has_value());
}

TEST(value_test, struct_default_value) {
    auto&& st = t::struct_type("Point", {{"x", t::int_type()}, {"y", t::int_type()}});
    auto&& val = v::default_value(st);
    EXPECT_TRUE(val.is_struct());
    auto&& s = val.to_struct();
    EXPECT_EQ(s.fields_.size(), 2);
    EXPECT_EQ(s.fields_[0].to_int(), 0);
    EXPECT_EQ(s.fields_[1].to_int(), 0);
    EXPECT_EQ(s.type_.struct_name(), "Point");
}

TEST(value_test, struct_equality) {
    auto&& st1 = t::struct_type("Point", {{"x", t::int_type()}, {"y", t::int_type()}});
    auto&& st2 = t::struct_type("Point", {{"x", t::int_type()}, {"y", t::int_type()}});
    auto&& st3 = t::struct_type("Point3D", {{"x", t::int_type()}, {"y", t::int_type()}, {"z", t::int_type()}});
    EXPECT_EQ(st1, st2);
    EXPECT_NE(st1, st3);
}

TEST(value_test, struct_nested_default_value) {
    auto&& point_t = t::struct_type("Point", {{"x", t::int_type()}, {"y", t::int_type()}});
    auto&& rect_t = t::struct_type("Rect", {{"tl", point_t}, {"br", point_t}});
    auto&& val = v::default_value(rect_t);
    EXPECT_TRUE(val.is_struct());
    auto&& r = val.to_struct();
    EXPECT_EQ(r.fields_.size(), 2);
    EXPECT_TRUE(r.fields_[0].is_struct());
    EXPECT_TRUE(r.fields_[1].is_struct());
}

TEST(value_test, from_string_int) {
    auto&& val = v::from_string("42", false);
    EXPECT_TRUE(val.is_int());
    EXPECT_EQ(val.to_int(), 42);
}

TEST(value_test, from_string_double) {
    auto&& val = v::from_string("3.14", true);
    EXPECT_TRUE(val.is_double());
    EXPECT_DOUBLE_EQ(val.to_double(), 3.14);
}

TEST(value_test, from_string_invalid) {
    EXPECT_THROW(std::ignore = v::from_string("abc", false), core::value_error);
}

}  // namespace cint::tests
