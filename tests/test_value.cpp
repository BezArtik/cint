// tests/test_value.cpp

#include "core/error/error_codes.hpp"
#include "core/value/operations.hpp"
#include "core/value/value.hpp"

#include <gtest/gtest.h>
#include <string>

namespace tests {

using t = core::type;
using v = core::value;
namespace op = core::ops;

TEST(value_test, default_is_void) {
    v val{};
    EXPECT_TRUE(val.is_void());
}

TEST(value_test, int_value) {
    v val{v::int_t{42}};
    EXPECT_EQ(val.to_int(), 42);
}

TEST(value_test, double_value) {
    v val{3.14};
    EXPECT_DOUBLE_EQ(val.to_double(), 3.14);
}

TEST(value_test, bool_value) {
    v val{true};
    EXPECT_EQ(val.to_bool(), true);
}

TEST(value_test, string_value) {
    v val{std::string{"hello"}};
    EXPECT_EQ(val.to_string(), "hello");
}

TEST(value_test, int_to_string) {
    v val{v::int_t{42}};
    EXPECT_EQ(val.to_string(), "42");
}

TEST(value_test, bool_to_string) {
    v val{true};
    EXPECT_EQ(val.to_string(), "true");
}

TEST(value_test, void_to_string) {
    v val{};
    EXPECT_EQ(val.to_string(), "void");
}

TEST(value_test, int_to_double) {
    v val{v::int_t{42}};
    EXPECT_DOUBLE_EQ(val.to_double(), 42.0);
}

TEST(value_test, double_to_int) {
    v val{3.14};
    EXPECT_EQ(val.to_int(), 3);
}

TEST(value_test, add_ints) {
    v a{v::int_t{10}};
    v b{v::int_t{20}};
    EXPECT_EQ(op::add(a, b).to_int(), 30);
}

TEST(value_test, add_int_and_double) {
    v a{v::int_t{10}};
    v b{3.5};
    EXPECT_DOUBLE_EQ(op::add(a, b).to_double(), 13.5);
}

TEST(value_test, sub_ints) {
    v a{v::int_t{30}};
    v b{v::int_t{10}};
    EXPECT_EQ(op::sub(a, b).to_int(), 20);
}

TEST(value_test, mul_ints) {
    v a{v::int_t{6}};
    v b{v::int_t{7}};
    EXPECT_EQ(op::mul(a, b).to_int(), 42);
}

TEST(value_test, div_ints) {
    v a{v::int_t{20}};
    v b{v::int_t{4}};
    EXPECT_EQ(op::div(a, b).to_int(), 5);
}

TEST(value_test, div_by_zero) {
    v a{v::int_t{1}};
    v b{v::int_t{0}};
    EXPECT_THROW(op::div(a, b), core::value_error);
}

TEST(value_test, mod_ints) {
    v a{v::int_t{17}};
    v b{v::int_t{5}};
    EXPECT_EQ(op::mod(a, b).to_int(), 2);
}

TEST(value_test, mod_by_zero) {
    v a{v::int_t{1}};
    v b{v::int_t{0}};
    EXPECT_THROW(op::mod(a, b), core::value_error);
}

TEST(value_test, eq_ints) {
    v a{v::int_t{42}};
    v b{v::int_t{42}};
    v c{v::int_t{0}};
    EXPECT_TRUE(op::eq(a, b).to_bool());
    EXPECT_FALSE(op::eq(a, c).to_bool());
}

TEST(value_test, lt_ints) {
    v a{v::int_t{10}};
    v b{v::int_t{20}};
    EXPECT_TRUE(op::lt(a, b).to_bool());
    EXPECT_FALSE(op::lt(b, a).to_bool());
}

TEST(value_test, not_op) {
    v t{true};
    v f{false};
    EXPECT_FALSE(op::not_op(t).to_bool());
    EXPECT_TRUE(op::not_op(f).to_bool());
}

TEST(value_test, unary_minus_int) {
    v a{v::int_t{42}};
    EXPECT_EQ(op::unary_minus(a).to_int(), -42);
}

TEST(value_test, bit_and) {
    v a{v::int_t{0b1100}};
    v b{v::int_t{0b1010}};
    EXPECT_EQ(op::bit_and(a, b).to_int(), 0b1000);
}

TEST(value_test, bit_or) {
    v a{v::int_t{0b1001}};
    v b{v::int_t{0b0110}};
    EXPECT_EQ(op::bit_or(a, b).to_int(), 0b1111);
}

TEST(value_test, bit_xor) {
    v a{v::int_t{0b1011}};
    v b{v::int_t{0b0110}};
    EXPECT_EQ(op::bit_xor(a, b).to_int(), 0b1101);
}

TEST(value_test, bit_not) {
    v a{v::int_t{10}};
    EXPECT_EQ(op::bit_not(a).to_int(), -11);
}

TEST(value_test, shl) {
    v a{v::int_t{0b0001}};
    v b{v::int_t{2}};
    EXPECT_EQ(op::shl(a, b).to_int(), 0b0100);
}

TEST(value_test, shr) {
    v a{v::int_t{0b1000}};
    v b{v::int_t{3}};
    EXPECT_EQ(op::shr(a, b).to_int(), 0b0001);
}

TEST(value_test, invalid_add) {
    v a{true};
    v b{v::int_t{1}};
    EXPECT_THROW(op::add(a, b), core::value_error);
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
    auto&& s = val.as<v::struct_t>();
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->fields_.size(), 2);
    EXPECT_EQ(s->fields_[0].to_int(), 0);
    EXPECT_EQ(s->fields_[1].to_int(), 0);
    EXPECT_EQ(s->type_.struct_name(), "Point");
}

TEST(value_test, struct_to_string) {
    auto&& st = t::struct_type("Point", {{"x", t::int_type()}, {"y", t::int_type()}});
    auto&& val = v::default_value(st);
    EXPECT_EQ(val.to_string(), "{0, 0}");
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
    auto&& r = val.as<v::struct_t>();
    EXPECT_EQ(r->fields_.size(), 2);
    EXPECT_TRUE(r->fields_[0].is_struct());
    EXPECT_TRUE(r->fields_[1].is_struct());
}

TEST(value_test, convert_int_to_double) {
    v val{v::int_t{42}};
    auto&& converted = v::convert(val, t::double_type());
    EXPECT_TRUE(converted.is_double());
    EXPECT_DOUBLE_EQ(converted.to_double(), 42.0);
}

TEST(value_test, convert_same_type) {
    v val{v::int_t{42}};
    auto&& converted = v::convert(val, t::int_type());
    EXPECT_EQ(converted.to_int(), 42);
}

TEST(value_test, from_string_int) {
    auto val = v::from_string("42", false);
    EXPECT_TRUE(val.is_int());
    EXPECT_EQ(val.to_int(), 42);
}

TEST(value_test, from_string_double) {
    auto&& val = v::from_string("3.14", true);
    EXPECT_TRUE(val.is_double());
    EXPECT_DOUBLE_EQ(val.to_double(), 3.14);
}

TEST(value_test, from_string_invalid) {
    EXPECT_THROW(v::from_string("abc", false), core::value_error);
}

}  // namespace tests
