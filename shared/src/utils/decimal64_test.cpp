#include <utils/decimal64.hpp>

#include <sstream>

#include <gtest/gtest.h>

using Dec4 = decimal64::decimal<4>;

TEST(Decimal64, ToString) {
  ASSERT_EQ(decimal64::toString(Dec4{"1000"}), "1000.0000");
  ASSERT_EQ(decimal64::toString(Dec4{"0"}), "0.0000");
  ASSERT_EQ(decimal64::toString(Dec4{"1"}), "1.0000");
  ASSERT_EQ(decimal64::toString(Dec4{"1.1"}), "1.1000");
  ASSERT_EQ(decimal64::toString(Dec4{"1.01"}), "1.0100");
  ASSERT_EQ(decimal64::toString(Dec4{"1.001"}), "1.0010");
  ASSERT_EQ(decimal64::toString(Dec4{"1.0001"}), "1.0001");
  ASSERT_EQ(decimal64::toString(Dec4{"-20"}), "-20.0000");
  ASSERT_EQ(decimal64::toString(Dec4{"-0.1"}), "-0.1000");
  ASSERT_EQ(decimal64::toString(Dec4{"-0.0001"}), "-0.0001");
  ASSERT_EQ(decimal64::toString(decimal64::decimal<5>{"1"}), "1.00000");
}

TEST(Decimal64, DefaultValue) { ASSERT_EQ(Dec4{}, Dec4{0}); }

TEST(Decimal64, DefaultRoundingPolicy) {
  ASSERT_EQ(Dec4{"1.23456"}, Dec4{"1.2346"});

  Dec4 out;
  std::istringstream is2{"1.0000000000000000000000000000000000077777777777 2"};
  ASSERT_TRUE(decimal64::fromStream(is2, out));
  ASSERT_EQ(out, Dec4{1});

  ASSERT_TRUE(decimal64::fromString("1.00006", out));
  ASSERT_EQ(out, Dec4{1.0001});
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
