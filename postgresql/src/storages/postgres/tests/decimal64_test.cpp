#include <gtest/gtest.h>

#include <decimal64/decimal64.hpp>

namespace {

using Dec4 = decimal64::decimal<4>;

}  // namespace

TEST(Decimal64, FromString) { ASSERT_EQ(Dec4{"10"}, Dec4{10}); }

TEST(Decimal64, ToString) {
  ASSERT_EQ(decimal64::toString(decimal64::decimal<4>{"10"}), "10.0000");
  ASSERT_EQ(decimal64::toString(decimal64::decimal<5>{"10"}), "10.00000");
}

TEST(Decimal64, DefaultValue) { ASSERT_EQ(Dec4{}, Dec4{0}); }

TEST(Decimal64, DefaultRoundingPolicy) {
  ASSERT_EQ(Dec4{"1.23456"}, Dec4{"1.2346"});
}

TEST(Decimal64, ArithmeticOperations) {
  Dec4 value{0};
  value += Dec4{4};
  value -= Dec4{1};
  value *= Dec4{30};
  value /= Dec4{9};
  ASSERT_EQ(value, Dec4{10});
}

TEST(Decimal64, OperationsWithSign) {
  ASSERT_EQ(Dec4{0}, Dec4{"0.0001"} + Dec4{"-0.0001"});
}

TEST(Decimal64, MoneyExample) {
  using Money = decimal64::decimal<4>;

  Money jpy_10k_to_rub{"5970.77"};
  Money value_rub{"10.00"};
  Money value_jpy = value_rub * jpy_10k_to_rub / Money{"10000.00"};
  ASSERT_EQ(decimal64::toString(value_jpy), "5.9708");
}
