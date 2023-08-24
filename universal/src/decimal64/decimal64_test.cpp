#include <userver/decimal64/decimal64.hpp>

#include <limits>
#include <unordered_map>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

using Dec2 = decimal64::Decimal<2>;
using Dec4 = decimal64::Decimal<4>;

template <typename RoundPolicy>
class Decimal64Round : public ::testing::Test {
 public:
  using Dec = decimal64::Decimal<4, RoundPolicy>;
};
using RoundPolicies = ::testing::Types<
    decimal64::DefRoundPolicy, decimal64::HalfDownRoundPolicy,
    decimal64::HalfUpRoundPolicy, decimal64::HalfEvenRoundPolicy,
    decimal64::CeilingRoundPolicy, decimal64::FloorRoundPolicy,
    decimal64::RoundDownRoundPolicy, decimal64::RoundUpRoundPolicy>;
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

TEST(Decimal64, ArithmeticOperationsMixedPrecision) {
  Dec4 value{0};
  value += Dec2{2};
  value -= Dec2{1};
  value *= Dec2{6};
  value /= Dec2{2};
  EXPECT_EQ(value, Dec4{3});

  static_assert(std::is_same_v<decltype(Dec4{} + Dec2{}), Dec4>);
  static_assert(std::is_same_v<decltype(Dec4{} - Dec2{}), Dec4>);
  static_assert(std::is_same_v<decltype(Dec4{} * Dec2{}), Dec4>);
  static_assert(std::is_same_v<decltype(Dec4{} / Dec2{}), Dec4>);

  static_assert(std::is_same_v<decltype(Dec2{} + Dec4{}), Dec4>);
  static_assert(std::is_same_v<decltype(Dec2{} - Dec4{}), Dec4>);

  // TODO TAXICOMMON-3727 fix confusing result types
  static_assert(std::is_same_v<decltype(Dec2{} * Dec4{}), Dec2>);
  static_assert(std::is_same_v<decltype(Dec2{} / Dec4{}), Dec2>);

  EXPECT_EQ(Dec4{3} + Dec2{2}, Dec4{5});
  EXPECT_EQ(Dec4{3} - Dec2{2}, Dec4{1});
  EXPECT_EQ(Dec4{3} * Dec2{2}, Dec4{6});
  EXPECT_EQ(Dec4{3} / Dec2{2}, Dec4{"1.5"});

  EXPECT_EQ(Dec2{3} + Dec4{2}, Dec4{5});
  EXPECT_EQ(Dec2{3} - Dec4{2}, Dec4{1});

  // TODO TAXICOMMON-3727 fix confusing result types
  EXPECT_EQ(Dec2{3} * Dec4{2}, Dec2{6});
  EXPECT_EQ(Dec2{3} / Dec4{2}, Dec2{"1.5"});
}

TEST(Decimal64, ArithmeticOperationsWithInt) {
  EXPECT_EQ(Dec4{6} + 2, Dec4{8});
  EXPECT_EQ(Dec4{6} - 2, Dec4{4});
  EXPECT_EQ(Dec4{6} * 2, Dec4{12});
  EXPECT_EQ(Dec4{6} / 2, Dec4{3});

  EXPECT_EQ(6 + Dec4{2}, Dec4{8});
  EXPECT_EQ(6 - Dec4{2}, Dec4{4});
  EXPECT_EQ(6 * Dec4{2}, Dec4{12});
  EXPECT_EQ(6 / Dec4{2}, Dec4{3});

  Dec4 value{0};
  value += 5;
  value -= 2;
  value *= 2;
  value /= 3;
  EXPECT_EQ(value, Dec4{2});
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
  [[maybe_unused]] constexpr Dec4 large_int{123456789876543LL};
  [[maybe_unused]] constexpr Dec4 from_string{"42.1234"};
  [[maybe_unused]] constexpr Dec4 from_float =
      Dec4::FromFloatInexact(42.123456F);
  [[maybe_unused]] constexpr Dec4 from_double =
      Dec4::FromFloatInexact(42.123456);
  [[maybe_unused]] constexpr Dec4 from_long_double =
      Dec4::FromFloatInexact(42.123456L);
  [[maybe_unused]] constexpr Dec4 from_unbiased = Dec4::FromUnbiased(123);
  [[maybe_unused]] constexpr Dec4 from_floating = Dec4::FromBiased(123, 5);
}

TEST(Decimal64, Hash) {
  std::unordered_map<Dec4, int> map{{Dec4{1}, 2}, {Dec4{3}, 4}};
  EXPECT_EQ(map[Dec4{1}], 2);
  EXPECT_EQ(map[Dec4{3}], 4);
}

TEST(Decimal64, IsDecimal) {
  EXPECT_TRUE(decimal64::kIsDecimal<Dec4>);
  EXPECT_TRUE((decimal64::kIsDecimal<
               decimal64::Decimal<0, decimal64::RoundDownRoundPolicy>>));
  EXPECT_FALSE(decimal64::kIsDecimal<const Dec4>);
  EXPECT_FALSE(decimal64::kIsDecimal<Dec4&>);
  EXPECT_FALSE(decimal64::kIsDecimal<const Dec4&>);
  EXPECT_FALSE(decimal64::kIsDecimal<int>);
}

TEST(Decimal64, Overflow) {
  constexpr Dec4 half_limit{"500000000000000"};
  constexpr Dec4 sqrt_limit{1'000'000'000};
  constexpr Dec4 min_decimal =
      Dec4::FromUnbiased(std::numeric_limits<int64_t>::min());

  EXPECT_THROW(half_limit + half_limit, decimal64::OutOfBoundsError);
  EXPECT_THROW((-half_limit) + (-half_limit), decimal64::OutOfBoundsError);
  EXPECT_THROW((-half_limit) - half_limit, decimal64::OutOfBoundsError);
  EXPECT_THROW(sqrt_limit * sqrt_limit, decimal64::OutOfBoundsError);
  EXPECT_THROW(sqrt_limit * sqrt_limit.ToInteger(),
               decimal64::OutOfBoundsError);
  EXPECT_THROW(min_decimal / -1, decimal64::OutOfBoundsError);
  EXPECT_THROW(min_decimal / Dec4{-1}, decimal64::OutOfBoundsError);
  EXPECT_THROW(-min_decimal, decimal64::OutOfBoundsError);
  EXPECT_THROW(Dec4{std::numeric_limits<uint64_t>::max()},
               decimal64::OutOfBoundsError);
}

TEST(Decimal64, DivisionByZero) {
  EXPECT_THROW(Dec4{1} / Dec4{0}, decimal64::DivisionByZeroError);
  EXPECT_THROW(Dec4{1} / Dec4::FromStringPermissive("0.00001"),
               decimal64::DivisionByZeroError);
  EXPECT_THROW(Dec4{1} / 0, decimal64::DivisionByZeroError);
}

TEST(Decimal64, RoundToMultipleOf) {
  const auto dec = Dec4{"12.346"};
  const auto max_decimal =
      Dec4::FromUnbiased(std::numeric_limits<int64_t>::max());

  EXPECT_EQ(dec.RoundToMultipleOf(Dec4{"0.0001"}), dec);
  EXPECT_EQ(dec.RoundToMultipleOf(Dec4{"0.001"}), dec);
  EXPECT_EQ(dec.RoundToMultipleOf(Dec4{"0.01"}), Dec4{"12.35"});
  EXPECT_EQ(dec.RoundToMultipleOf(Dec4{"0.1"}), Dec4{"12.3"});
  EXPECT_EQ(dec.RoundToMultipleOf(Dec4{1}), Dec4{12});
  EXPECT_EQ(dec.RoundToMultipleOf(Dec4{10}), Dec4{10});
  EXPECT_EQ(dec.RoundToMultipleOf(Dec4{100}), Dec4{0});

  EXPECT_EQ(dec.RoundToMultipleOf(Dec4{"0.2"}), Dec4{"12.4"});
  EXPECT_EQ(dec.RoundToMultipleOf(Dec4{"0.3"}), Dec4{"12.3"});
  EXPECT_EQ(dec.RoundToMultipleOf(Dec4{"0.4"}), Dec4{"12.4"});
  EXPECT_EQ(dec.RoundToMultipleOf(Dec4{"0.5"}), Dec4{"12.5"});
  EXPECT_EQ(dec.RoundToMultipleOf(Dec4{"0.6"}), Dec4{"12.6"});
  EXPECT_EQ(dec.RoundToMultipleOf(Dec4{"1.1"}), Dec4{"12.1"});
  EXPECT_EQ(dec.RoundToMultipleOf(Dec4{2}), Dec4{12});
  EXPECT_EQ(dec.RoundToMultipleOf(Dec4{3}), Dec4{12});
  EXPECT_EQ(dec.RoundToMultipleOf(Dec4{5}), Dec4{10});
  EXPECT_EQ(dec.RoundToMultipleOf(Dec4{7}), Dec4{14});
  EXPECT_EQ(dec.RoundToMultipleOf(Dec4{25}), Dec4{0});

  EXPECT_EQ(Dec4{-7}.RoundToMultipleOf(Dec4{10}), Dec4{-10});
  EXPECT_EQ(Dec4{-27}.RoundToMultipleOf(Dec4{10}), Dec4{-30});

  EXPECT_THROW(dec.RoundToMultipleOf(Dec4{0}), decimal64::DivisionByZeroError);
  EXPECT_THROW(dec.RoundToMultipleOf(Dec4::FromStringPermissive("0.00001")),
               decimal64::DivisionByZeroError);
  EXPECT_THROW(max_decimal.RoundToMultipleOf(Dec4{10}),
               decimal64::OutOfBoundsError);
  EXPECT_THROW(dec.RoundToMultipleOf(Dec4{-1}), decimal64::OutOfBoundsError);
  EXPECT_THROW(Dec4{-7}.RoundToMultipleOf(Dec4{-10}),
               decimal64::OutOfBoundsError);
}

template <class T>
class Decimal64HalfRoundPolicy : public ::testing::Test {};

using HalfRoundPolicies = ::testing::Types<decimal64::HalfDownRoundPolicy,
                                           decimal64::HalfUpRoundPolicy,
                                           decimal64::HalfEvenRoundPolicy>;
TYPED_TEST_SUITE(Decimal64HalfRoundPolicy, HalfRoundPolicies);

TYPED_TEST(Decimal64HalfRoundPolicy, Division) {
  using Dec = decimal64::Decimal<0, TypeParam>;

  EXPECT_EQ(Dec{20} / Dec{10}, Dec{2});
  EXPECT_EQ(Dec{24} / Dec{10}, Dec{2});
  EXPECT_EQ(Dec{26} / Dec{10}, Dec{3});

  EXPECT_EQ(Dec{-20} / Dec{10}, Dec{-2});
  EXPECT_EQ(Dec{-24} / Dec{10}, Dec{-2});
  EXPECT_EQ(Dec{-26} / Dec{10}, Dec{-3});

  EXPECT_EQ(Dec{20} / Dec{-10}, Dec{-2});
  EXPECT_EQ(Dec{24} / Dec{-10}, Dec{-2});
  EXPECT_EQ(Dec{26} / Dec{-10}, Dec{-3});

  EXPECT_EQ(Dec{-20} / Dec{-10}, Dec{2});
  EXPECT_EQ(Dec{-24} / Dec{-10}, Dec{2});
  EXPECT_EQ(Dec{-26} / Dec{-10}, Dec{3});

  EXPECT_EQ(Dec{20} / Dec{5}, Dec{4});
  EXPECT_EQ(Dec{22} / Dec{5}, Dec{4});
  EXPECT_EQ(Dec{23} / Dec{5}, Dec{5});

  EXPECT_EQ(Dec{-20} / Dec{5}, Dec{-4});
  EXPECT_EQ(Dec{-22} / Dec{5}, Dec{-4});
  EXPECT_EQ(Dec{-23} / Dec{5}, Dec{-5});

  EXPECT_EQ(Dec{20} / Dec{-5}, Dec{-4});
  EXPECT_EQ(Dec{22} / Dec{-5}, Dec{-4});
  EXPECT_EQ(Dec{23} / Dec{-5}, Dec{-5});

  EXPECT_EQ(Dec{-20} / Dec{-5}, Dec{4});
  EXPECT_EQ(Dec{-22} / Dec{-5}, Dec{4});
  EXPECT_EQ(Dec{-23} / Dec{-5}, Dec{5});

  if constexpr (std::is_same_v<TypeParam, decimal64::HalfDownRoundPolicy> ||
                std::is_same_v<TypeParam, decimal64::HalfEvenRoundPolicy>) {
    EXPECT_EQ(Dec{25} / Dec{10}, Dec{2});
    EXPECT_EQ(Dec{-25} / Dec{10}, Dec{-2});
    EXPECT_EQ(Dec{25} / Dec{-10}, Dec{-2});
    EXPECT_EQ(Dec{-25} / Dec{-10}, Dec{2});
  } else {
    EXPECT_EQ(Dec{25} / Dec{10}, Dec{3});
    EXPECT_EQ(Dec{-25} / Dec{10}, Dec{-3});
    EXPECT_EQ(Dec{25} / Dec{-10}, Dec{-3});
    EXPECT_EQ(Dec{-25} / Dec{-10}, Dec{3});
  }

  if constexpr (std::is_same_v<TypeParam, decimal64::HalfDownRoundPolicy>) {
    EXPECT_EQ(Dec{35} / Dec{10}, Dec{3});
    EXPECT_EQ(Dec{-35} / Dec{10}, Dec{-3});
    EXPECT_EQ(Dec{35} / Dec{-10}, Dec{-3});
    EXPECT_EQ(Dec{-35} / Dec{-10}, Dec{3});
  } else {
    EXPECT_EQ(Dec{35} / Dec{10}, Dec{4});
    EXPECT_EQ(Dec{-35} / Dec{10}, Dec{-4});
    EXPECT_EQ(Dec{35} / Dec{-10}, Dec{-4});
    EXPECT_EQ(Dec{-35} / Dec{-10}, Dec{4});
  }
}

USERVER_NAMESPACE_END
