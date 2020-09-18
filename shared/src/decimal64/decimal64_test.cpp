#include <decimal64/decimal64.hpp>

#include <sstream>
#include <unordered_map>

#include <gtest/gtest.h>

using Dec4 = decimal64::Decimal<4>;

template <typename RoundPolicy>
class Decimal64Round : public ::testing::Test {
 public:
  using Dec = decimal64::Decimal<4, RoundPolicy>;
};
using RoundPolicies = ::testing::Types<
    decimal64::NullRoundPolicy, decimal64::DefRoundPolicy,
    decimal64::HalfDownRoundPolicy, decimal64::HalfUpRoundPolicy,
    decimal64::HalfEvenRoundPolicy, decimal64::CeilingRoundPolicy,
    decimal64::FloorRoundPolicy, decimal64::RoundDownRoundPolicy,
    decimal64::RoundUpRoundPolicy>;
TYPED_TEST_SUITE(Decimal64Round, RoundPolicies);

TYPED_TEST(Decimal64Round, FromDouble) {
  using Dec = typename TestFixture::Dec;
  constexpr auto kIterations = 10000;

  for (int64_t i = 0; i < kIterations; ++i) {
    const auto i1 = static_cast<float>(i) / Dec4::kDecimalFactor;
    const auto i2 = static_cast<double>(i) / Dec4::kDecimalFactor;
    const auto i3 = static_cast<long double>(i) / Dec4::kDecimalFactor;
    EXPECT_EQ(Dec::FromFloatInexact(i1).AsUnbiased(), i);
    EXPECT_EQ(Dec::FromFloatInexact(i2).AsUnbiased(), i);
    EXPECT_EQ(Dec::FromFloatInexact(i3).AsUnbiased(), i);
  }
}

TEST(Decimal64, DefaultValue) { ASSERT_EQ(Dec4{}, Dec4{0}); }

TEST(Decimal64, DefaultRoundingPolicy) {
  EXPECT_EQ(Dec4::FromStringPermissive("1.23456"), Dec4{"1.2346"});
  EXPECT_EQ(Dec4::FromStringPermissive(
                "1.0000000000000000000000000000000000077777777777"),
            Dec4{1});
  EXPECT_EQ(Dec4::FromStringPermissive("1.00006"), Dec4{"1.0001"});
}

TEST(Decimal64, ArithmeticOperations) {
  Dec4 value{0};
  value += Dec4{4};
  value -= Dec4{1};
  value *= Dec4{30};
  value /= Dec4{9};
  EXPECT_EQ(value, Dec4{10});
}

TEST(Decimal64, OperationsWithSign) {
  EXPECT_EQ(Dec4{0}, Dec4{"0.0001"} + Dec4{"-0.0001"});
}

TEST(Decimal64, MoneyExample) {
  using Money = decimal64::Decimal<4>;

  Money jpy_10k_to_rub{"5970.77"};
  Money value_rub{"10.00"};
  Money value_jpy = value_rub * jpy_10k_to_rub / Money{"10000.00"};
  EXPECT_EQ(decimal64::ToString(value_jpy), "5.9708");
}

TEST(Decimal64, ConstexprSupport) {
  [[maybe_unused]] constexpr Dec4 default_zero;
  [[maybe_unused]] constexpr Dec4 zero{0};
  [[maybe_unused]] constexpr Dec4 ten{10};
  [[maybe_unused]] constexpr Dec4 large_int{123456789876543};
  [[maybe_unused]] constexpr Dec4 from_string{"42.123456"};
  [[maybe_unused]] constexpr Dec4 from_float =
      Dec4::FromFloatInexact(42.123456F);
  [[maybe_unused]] constexpr Dec4 from_double =
      Dec4::FromFloatInexact(42.123456);
  [[maybe_unused]] constexpr Dec4 from_long_double =
      Dec4::FromFloatInexact(42.123456L);
  [[maybe_unused]] constexpr Dec4 from_unbiased = Dec4::FromUnbiased(123);
  [[maybe_unused]] constexpr Dec4 from_unpacked = Dec4::FromUnpacked(123, 4567);
  [[maybe_unused]] constexpr Dec4 from_unpacked2 =
      Dec4::FromUnpacked(123, 4567, 8);
  [[maybe_unused]] constexpr Dec4 from_floating = Dec4::FromBiased(123, 5);
  [[maybe_unused]] constexpr Dec4 from_fraction = Dec4::FromFraction(123, 456);
}

TEST(Decimal64, Hash) {
  std::unordered_map<Dec4, int> map{{Dec4{1}, 2}, {Dec4{3}, 4}};
  EXPECT_EQ(map[Dec4{1}], 2);
  EXPECT_EQ(map[Dec4{3}], 4);
}
