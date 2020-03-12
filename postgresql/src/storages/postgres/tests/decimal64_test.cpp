#include <gtest/gtest.h>

#include <decimal64/decimal64.hpp>

using Dec4 = decimal64::decimal<4>;

TEST(Decimal64, FromString) {
  ASSERT_EQ(Dec4{"10"}, Dec4{10});
  ASSERT_EQ(Dec4{"10."}, Dec4{10});

  Dec4 out;
  ASSERT_TRUE(decimal64::fromString("   10   ", out));
  ASSERT_EQ(out, Dec4{10});

  ASSERT_FALSE(decimal64::fromString("", out));
  ASSERT_FALSE(decimal64::fromString("10a", out));
  ASSERT_TRUE(decimal64::fromString(".0", out));
  ASSERT_TRUE(decimal64::fromString(".0 ", out));

  std::istringstream is1{"123ab"};
  ASSERT_TRUE(decimal64::fromStream(is1, out));
  ASSERT_EQ(out, Dec4{123});
  ASSERT_EQ(is1.get(), 'a');

  std::istringstream is2{"1.0000000000000000000000000000000000077777777777 2"};
  ASSERT_TRUE(decimal64::fromStream(is2, out));
  ASSERT_EQ(out, Dec4{1});
  ASSERT_EQ(is2.get(), ' ');
}

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
