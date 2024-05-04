#include <gmock/gmock.h>
#include <userver/utest/assert_macros.hpp>

#include <userver/utils/numeric_cast.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

TEST(NumericCast, Smoke) {
  EXPECT_EQ(utils::numeric_cast<unsigned int>(1), 1u);
  EXPECT_EQ(utils::numeric_cast<std::size_t>(1), static_cast<std::size_t>(1));
}

TEST(NumericCast, SignedToUnsignedOverflow) {
  EXPECT_EQ(utils::numeric_cast<std::uint16_t>(0xffff), 0xffffu);
  EXPECT_THROW(utils::numeric_cast<std::uint16_t>(0x10000), std::runtime_error);

  EXPECT_EQ(utils::numeric_cast<std::uint16_t>(0), 0);
  EXPECT_THROW(utils::numeric_cast<std::uint16_t>(-1), std::runtime_error);
}

TEST(NumericCast, UnsignedToSignedOverflow) {
  EXPECT_EQ(utils::numeric_cast<std::int16_t>(0), 0);
  EXPECT_THROW(utils::numeric_cast<std::int16_t>(0xffff), std::runtime_error);

  EXPECT_EQ(utils::numeric_cast<std::int16_t>(0x7fff), 0x7fff);
  EXPECT_THROW(utils::numeric_cast<std::int16_t>(0x8000), std::runtime_error);
}

}  // namespace

USERVER_NAMESPACE_END
