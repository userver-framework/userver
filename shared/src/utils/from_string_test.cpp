#include <userver/utils/from_string.hpp>

#include <limits>
#include <random>
#include <type_traits>

#include <gtest/gtest.h>
#include <boost/lexical_cast.hpp>

#include <userver/compiler/demangle.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

// std::to_string is unusable, because it does not keep the full precision of
// floating-point types
template <typename T>
std::string ToString(T value) {
  if constexpr (sizeof(value) == 1) {
    // Prevent printing int8_t and uint8_t as a character
    return std::to_string(static_cast<int>(value));
  } else {
    return boost::lexical_cast<std::string>(value);
  }
}

template <typename T>
auto TestInvalid(const std::string& input) {
  ASSERT_THROW(utils::FromString<T>(input), std::runtime_error)
      << "type = " << compiler::GetTypeName<T>() << ", input = \"" << input
      << "\"";
}

template <typename T>
auto TestConverts(const std::string& input, T expectedResult) {
  T actualResult{};
  ASSERT_NO_THROW(actualResult = utils::FromString<T>(input))
      << "type = " << compiler::GetTypeName<T>() << ", input = \"" << input
      << "\"";
  ASSERT_EQ(actualResult, expectedResult)
      << "type = " << compiler::GetTypeName<T>() << ", input = \"" << input
      << "\"";
}

template <typename T>
auto TestPreserves(T value) {
  TestConverts(ToString(value), value);
}

template <typename T>
auto DistributionForTesting() {
  if constexpr (std::is_floating_point_v<T>) {
    return std::normal_distribution<T>();
  } else {
    return std::uniform_int_distribution<T>(std::numeric_limits<T>::min(),
                                            std::numeric_limits<T>::max());
  }
}

}  // namespace

template <typename T>
class FromStringTest : public ::testing::Test {};

using NumericTypes =
    ::testing::Types<int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t,
                     int64_t, uint64_t, float, double, long double>;
TYPED_TEST_SUITE(FromStringTest, NumericTypes);

TYPED_TEST(FromStringTest, Sign) {
  using T = TypeParam;

  TestConverts("0", T{0});
  TestConverts("+0", T{0});
  TestConverts("-0", T{0});
  TestConverts("+1", T{1});

  if constexpr (std::is_signed_v<T>) {
    TestConverts("-1", T{-1});
  }
}

TYPED_TEST(FromStringTest, Randomized) {
  using T = TypeParam;
  constexpr int kTestIterations = 100;

  // `randomEngine` is initialized with a fixed default seed
  // NOLINTNEXTLINE(cert-msc51-cpp)
  std::default_random_engine randomEngine;
  auto distribution = DistributionForTesting<T>();

  for (int i = 0; i < kTestIterations; ++i) {
    TestPreserves(distribution(randomEngine));
  }
}

TYPED_TEST(FromStringTest, Limits) {
  using T = TypeParam;

  TestPreserves(std::numeric_limits<T>::min());
  TestPreserves(std::numeric_limits<T>::max());

  if constexpr (std::is_floating_point_v<T>) {
    TestPreserves(std::numeric_limits<T>::lowest());
    TestPreserves(-std::numeric_limits<T>::lowest());
    TestPreserves(std::numeric_limits<T>::infinity());
    TestPreserves(-std::numeric_limits<T>::infinity());
    ASSERT_TRUE(std::isnan(utils::FromString<T>("nan")));
  }
}

TYPED_TEST(FromStringTest, StrangeDecimalPoints) {
  using T = TypeParam;

  if constexpr (std::is_floating_point_v<T>) {
    TestConverts(".1", T{0.1L});
    TestConverts("1.", T{1.0});
    TestInvalid<T>("..");
  }
}

TYPED_TEST(FromStringTest, Exponents) {
  using T = TypeParam;

  if constexpr (std::is_floating_point_v<T>) {
    TestConverts("0e0", T{0e0});
    TestConverts("12e3", T{12e3});
    TestConverts("-12e3", T{-12e3});
  }
}

TYPED_TEST(FromStringTest, NonDecimal) {
  using T = TypeParam;

  TestConverts("010", T{10});
  TestConverts("0000000000000000000000000000000010", T{10});

  TestInvalid<T>("0b10");
  TestInvalid<T>("0o10");

  if constexpr (std::is_floating_point_v<T>) {
    TestConverts("0x10", T{0x10});
    TestConverts("0xAB", T{0xAB});
    TestConverts("0xab", T{0xAB});
    TestConverts("0xABP2", T{0xABP2});
  } else {
    TestInvalid<T>("0x10");
  }
}

TYPED_TEST(FromStringTest, ExtraSpaces) {
  using T = TypeParam;

  TestInvalid<T>(" 1");
  TestInvalid<T>("1 ");
  TestInvalid<T>("1\t");
  TestInvalid<T>("1\n");
  TestInvalid<T>("1\r");
  TestInvalid<T>("1\v");
}

TYPED_TEST(FromStringTest, ExtraGarbage) {
  using T = TypeParam;

  TestInvalid<T>("#");
  TestInvalid<T>("#1");
  TestInvalid<T>("1#");
  TestInvalid<T>("1 #");
  TestInvalid<T>("1-");
  TestInvalid<T>("a1");
  TestInvalid<T>("1a");

  if constexpr (!std::is_floating_point_v<T>) {
    TestInvalid<T>("1.0");
    TestInvalid<T>("1.");
    TestInvalid<T>(".1");
  }
}

TYPED_TEST(FromStringTest, Overflow) {
  using T = TypeParam;

  if constexpr (std::is_unsigned_v<T>) {
    TestInvalid<T>("-1");
  } else {
    TestInvalid<T>(ToString(std::numeric_limits<T>::min()) + "0");
  }
  TestInvalid<T>(ToString(std::numeric_limits<T>::max()) + "0");

  if constexpr (std::is_floating_point_v<T>) {
    TestInvalid<T>("1e1000000000000000000000000");
  } else {
    TestInvalid<T>("1000000000000000000000000");
  }
}

TYPED_TEST(FromStringTest, ExceptionDetails) {
  using T = TypeParam;
  std::string what;

  try {
    utils::FromString<T>(".blah");
  } catch (const std::runtime_error& e) {
    what = e.what();
  } catch (const std::exception& e) {
    // swallow
  }

  ASSERT_FALSE(what.empty());
  ASSERT_NE(what.find(".blah"), std::string::npos);
  ASSERT_NE(what.find(compiler::GetTypeName<T>()), std::string::npos);
}

USERVER_NAMESPACE_END
