#include <userver/utils/underlying_value.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace {

enum class IntEnum : int {};
enum class UnsignedEnum : unsigned {};

enum UnscopedEnum : short {
  kValue = 3,
};

}  // namespace

TEST(UnderlyingValue, Values) {
  EXPECT_EQ(1, utils::UnderlyingValue(IntEnum(1)));
  EXPECT_EQ(2, utils::UnderlyingValue(UnsignedEnum(2)));
  EXPECT_EQ(3, utils::UnderlyingValue(UnscopedEnum::kValue));
}

TEST(UnderlyingValue, EmptyStruct) {
  EXPECT_TRUE(
      (std::is_same<int, decltype(utils::UnderlyingValue(IntEnum(1)))>::value));
  EXPECT_TRUE((std::is_same<unsigned, decltype(utils::UnderlyingValue(
                                          UnsignedEnum(1)))>::value));
  EXPECT_TRUE((std::is_same<short, decltype(utils::UnderlyingValue(
                                       UnscopedEnum::kValue))>::value));
}

USERVER_NAMESPACE_END
