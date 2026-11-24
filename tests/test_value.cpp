// tests/test_value.cpp

#include "core/value/value.hpp"

#include <gtest/gtest.h>
#include <string>

namespace cint::tests {

using v = core::value;

TEST(value_test, default_is_void) {
  auto val = v{};
  EXPECT_TRUE(val.is_void());
  EXPECT_FALSE(val.to_bool());
  EXPECT_EQ(val.to_string(), "void");
}

TEST(value_test, primitives) {
  EXPECT_EQ(v{v::int_t{42}}.to_int(), 42);
  EXPECT_DOUBLE_EQ(v{3.14}.to_double(), 3.14);
  EXPECT_EQ(v{true}.to_bool(), true);
  EXPECT_EQ(v{std::string{"hello"}}.to_string(), "hello");
}

TEST(value_test, conversions) {
  EXPECT_EQ(v{v::int_t{42}}.to_double(), 42.0);
  EXPECT_EQ(v{3.14}.to_int(), 3);
  EXPECT_EQ(v{v::int_t{1}}.to_bool(), true);
  EXPECT_EQ(v{v::int_t{0}}.to_bool(), false);
  EXPECT_EQ(v{std::string{""}}.to_bool(), false);
  EXPECT_EQ(v{std::string{"x"}}.to_bool(), true);
  EXPECT_EQ(v{std::string{"42"}}.to_int(), 42);
}

TEST(value_test, to_bool_is_total) {
  EXPECT_FALSE(v{}.to_bool());
  EXPECT_TRUE(v{v::int_t{1}}.to_bool());
  EXPECT_FALSE(v{v::int_t{0}}.to_bool());
  EXPECT_TRUE(v{std::string{"x"}}.to_bool());
  EXPECT_FALSE(v{std::string{""}}.to_bool());
}

TEST(value_test, to_string_is_total) {
  EXPECT_EQ(v{v::int_t{42}}.to_string(), "42");
  EXPECT_EQ(v{true}.to_string(), "true");
  EXPECT_EQ(v{}.to_string(), "void");
}

TEST(value_test, add_ints) { EXPECT_EQ((v{v::int_t{10}} + v{v::int_t{20}}).to_int(), 30); }

TEST(value_test, add_int_and_double) { EXPECT_DOUBLE_EQ((v{v::int_t{10}} + v{3.5}).to_double(), 13.5); }

TEST(value_test, add_strings) { EXPECT_EQ((v{std::string{"ab"}} + v{std::string{"cd"}}).to_string(), "abcd"); }

TEST(value_test, sub_ints) { EXPECT_EQ((v{v::int_t{30}} - v{v::int_t{10}}).to_int(), 20); }

TEST(value_test, mul_ints) { EXPECT_EQ((v{v::int_t{6}} * v{v::int_t{7}}).to_int(), 42); }

TEST(value_test, mul_string_repetition) {
  EXPECT_EQ((v{std::string{"ab"}} * v{v::int_t{3}}).to_string(), "ababab");
  EXPECT_EQ((v{v::int_t{3}} * v{std::string{"ab"}}).to_string(), "ababab");
}

TEST(value_test, div_ints) { EXPECT_EQ((v{v::int_t{20}} / v{v::int_t{4}}).to_int(), 5); }

TEST(value_test, div_by_zero) { EXPECT_THROW(std::ignore = v{v::int_t{1}} / v{v::int_t{0}}, core::value_error); }

TEST(value_test, mod_ints) { EXPECT_EQ((v{v::int_t{17}} % v{v::int_t{5}}).to_int(), 2); }

TEST(value_test, mod_by_zero) { EXPECT_THROW(std::ignore = v{v::int_t{1}} % v{v::int_t{0}}, core::value_error); }

TEST(value_test, invalid_add) { EXPECT_THROW(std::ignore = v{true} + v{v::int_t{1}}, core::value_error); }

TEST(value_test, eq_ints) {
  EXPECT_TRUE(v{v::int_t{42}} == v{v::int_t{42}});
  EXPECT_FALSE(v{v::int_t{42}} == v{v::int_t{0}});
}

TEST(value_test, eq_int_double) { EXPECT_TRUE(v{v::int_t{1}} == v{1.0}); }

TEST(value_test, eq_strings) {
  EXPECT_TRUE(v{std::string{"a"}} == v{std::string{"a"}});
  EXPECT_FALSE(v{std::string{"a"}} == v{std::string{"b"}});
}

TEST(value_test, eq_incomparable_is_false) {
  EXPECT_FALSE(v{v::int_t{1}} == v{std::string{"1"}});
  EXPECT_FALSE(v{true} == v{v::int_t{1}});
}

TEST(value_test, lt_ints) {
  EXPECT_TRUE(v{v::int_t{10}} < v{v::int_t{20}});
  EXPECT_FALSE(v{v::int_t{20}} < v{v::int_t{10}});
}

TEST(value_test, not_op) {
  EXPECT_FALSE(!v{true});
  EXPECT_TRUE(!v{false});
  EXPECT_TRUE(!v{v::int_t{0}});
  EXPECT_FALSE(!v{v::int_t{1}});
  EXPECT_TRUE(!v{std::string{""}});
}

TEST(value_test, unary_minus) { EXPECT_EQ((-v{v::int_t{42}}).to_int(), -42); }

TEST(value_test, bit_ops) {
  EXPECT_EQ((v{v::int_t{0b1100}} & v{v::int_t{0b1010}}).to_int(), 0b1000);
  EXPECT_EQ((v{v::int_t{0b1001}} | v{v::int_t{0b0110}}).to_int(), 0b1111);
  EXPECT_EQ((v{v::int_t{0b1011}} ^ v{v::int_t{0b0110}}).to_int(), 0b1101);
  EXPECT_EQ((~v{v::int_t{10}}).to_int(), -11);
  EXPECT_EQ((v{v::int_t{1}} << v{v::int_t{2}}).to_int(), 4);
  EXPECT_EQ((v{v::int_t{8}} >> v{v::int_t{3}}).to_int(), 1);
}

}  // namespace cint::tests
