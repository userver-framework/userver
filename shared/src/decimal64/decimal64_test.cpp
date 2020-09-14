#include <decimal64/decimal64.hpp>

#include <sstream>

#include <gtest/gtest.h>

using Dec4 = decimal64::Decimal<4>;

TEST(Decimal64, ToString) {
  ASSERT_EQ(decimal64::ToString(Dec4{"1000"}), "1000");
  ASSERT_EQ(decimal64::ToString(Dec4{"0"}), "0");
  ASSERT_EQ(decimal64::ToString(Dec4{"1"}), "1");
  ASSERT_EQ(decimal64::ToString(Dec4{"1.1"}), "1.1");
  ASSERT_EQ(decimal64::ToString(Dec4{"1.01"}), "1.01");
  ASSERT_EQ(decimal64::ToString(Dec4{"1.001"}), "1.001");
  ASSERT_EQ(decimal64::ToString(Dec4{"1.0001"}), "1.0001");
  ASSERT_EQ(decimal64::ToString(Dec4{"-20"}), "-20");
  ASSERT_EQ(decimal64::ToString(Dec4{"-0.1"}), "-0.1");
  ASSERT_EQ(decimal64::ToString(Dec4{"-0.0001"}), "-0.0001");
  ASSERT_EQ(decimal64::ToString(decimal64::Decimal<18>{"1"}), "1");
  ASSERT_EQ(decimal64::ToString(decimal64::Decimal<5>{"1"}), "1");
  ASSERT_EQ(decimal64::ToString(decimal64::Decimal<0>{"1"}), "1");
}

TEST(Decimal64, ToStringTrailingZeros) {
  ASSERT_EQ(decimal64::ToStringTrailingZeros(Dec4{"1000"}), "1000.0000");
  ASSERT_EQ(decimal64::ToStringTrailingZeros(Dec4{"0"}), "0.0000");
  ASSERT_EQ(decimal64::ToStringTrailingZeros(Dec4{"1"}), "1.0000");
  ASSERT_EQ(decimal64::ToStringTrailingZeros(Dec4{"1.1"}), "1.1000");
  ASSERT_EQ(decimal64::ToStringTrailingZeros(Dec4{"1.01"}), "1.0100");
  ASSERT_EQ(decimal64::ToStringTrailingZeros(Dec4{"1.001"}), "1.0010");
  ASSERT_EQ(decimal64::ToStringTrailingZeros(Dec4{"1.0001"}), "1.0001");
  ASSERT_EQ(decimal64::ToStringTrailingZeros(Dec4{"-20"}), "-20.0000");
  ASSERT_EQ(decimal64::ToStringTrailingZeros(Dec4{"-0.1"}), "-0.1000");
  ASSERT_EQ(decimal64::ToStringTrailingZeros(Dec4{"-0.0001"}), "-0.0001");
  ASSERT_EQ(decimal64::ToStringTrailingZeros(decimal64::Decimal<18>{"1"}),
            "1.000000000000000000");
  ASSERT_EQ(decimal64::ToStringTrailingZeros(decimal64::Decimal<5>{"1"}),
            "1.00000");
  ASSERT_EQ(decimal64::ToStringTrailingZeros(decimal64::Decimal<0>{"1"}), "1");
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
  using Money = decimal64::Decimal<4>;

  Money jpy_10k_to_rub{"5970.77"};
  Money value_rub{"10.00"};
  Money value_jpy = value_rub * jpy_10k_to_rub / Money{"10000.00"};
  ASSERT_EQ(decimal64::ToString(value_jpy), "5.9708");
}

TEST(Decimal64, ConstexprSupport) {
  [[maybe_unused]] constexpr Dec4 default_zero;
  [[maybe_unused]] constexpr Dec4 zero{0};
  [[maybe_unused]] constexpr Dec4 ten{10};
  [[maybe_unused]] constexpr Dec4 large_int{123456789876543};
  [[maybe_unused]] constexpr Dec4 from_float{42.123456F};
  [[maybe_unused]] constexpr Dec4 from_double{42.123456};
  [[maybe_unused]] constexpr Dec4 from_long_double{42.123456L};
  [[maybe_unused]] constexpr Dec4 from_unbiased = Dec4::FromUnbiased(123);
  [[maybe_unused]] constexpr Dec4 from_unpacked = Dec4::FromUnpacked(123, 4567);
  [[maybe_unused]] constexpr Dec4 from_unpacked2 =
      Dec4::FromUnpacked(123, 4567, 8);
  [[maybe_unused]] constexpr Dec4 from_floating = Dec4::FromBiased(123, 5);
  [[maybe_unused]] constexpr Dec4 from_fraction = Dec4::FromFraction(123, 456);
}

TEST(Decimal64, ZerosFullyTrimmed) {
  EXPECT_EQ(ToString(decimal64::Decimal<0>{"1"}), "1");
  EXPECT_EQ(ToString(decimal64::Decimal<1>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<2>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<3>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<4>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<5>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<6>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<7>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<8>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<9>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<10>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<11>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<12>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<13>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<14>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<15>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<16>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<17>{"0.1"}), "0.1");
  EXPECT_EQ(ToString(decimal64::Decimal<18>{"0.1"}), "0.1");
}
