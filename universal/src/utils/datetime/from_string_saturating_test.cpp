#include <userver/utils/datetime/from_string_saturating.hpp>

#include <gtest/gtest.h>

#include <userver/utils/datetime.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

std::chrono::system_clock::time_point GetDateTimePlatformSpecificBigValue() {
#ifdef _LIBCPP_VERSION
  // MAC_COMPAT: std::chrono::system_clock::time_point in libc++ can handle
  // years greater than 9000. So the `From*StringSaturating` functions do not
  // saturate.
  return utils::datetime::FromRfc3339StringSaturating(
      "9999-12-31T00:00:00+0000");
#else
  return std::chrono::system_clock::time_point::max();
#endif
}

}  // namespace

TEST(FromStringSaturation, Rfc3339) {
  EXPECT_EQ(
      utils::datetime::FromRfc3339StringSaturating("9999-12-31T00:00:00+0000"),
      GetDateTimePlatformSpecificBigValue());

  EXPECT_EQ(
      utils::datetime::FromRfc3339StringSaturating("10000-01-01T00:00:00+0000"),
      std::chrono::system_clock::time_point::max());
}

TEST(FromStringSaturation, Formats) {
  EXPECT_EQ(utils::datetime::FromStringSaturating("9999-12-31T00:00:00Z",
                                                  "%Y-%m-%dT%H:%M:%SZ"),
            GetDateTimePlatformSpecificBigValue());

  EXPECT_EQ(utils::datetime::FromStringSaturating("9999-12-31", "%Y-%m-%d"),
            GetDateTimePlatformSpecificBigValue());

  EXPECT_EQ(utils::datetime::FromStringSaturating("10000-01-01", "%Y-%m-%d"),
            std::chrono::system_clock::time_point::max());

  EXPECT_EQ(utils::datetime::FromStringSaturating("01-01-10000", "%d-%m-%Y"),
            std::chrono::system_clock::time_point::max());
}

TEST(FromStringSaturation, Invalid) {
  EXPECT_THROW(utils::datetime::FromStringSaturating(
                   "12345", utils::datetime::kDefaultFormat),
               utils::datetime::DateParseError);
  EXPECT_THROW(utils::datetime::FromStringSaturating(
                   "99999-ABCDEF", utils::datetime::kDefaultFormat),
               utils::datetime::DateParseError);
  EXPECT_THROW(utils::datetime::FromStringSaturating(
                   "2020-12-45T00:00:00Z", utils::datetime::kDefaultFormat),
               utils::datetime::DateParseError);
  EXPECT_THROW(utils::datetime::FromStringSaturating(
                   "2021-01-22T17:14:00Zzzz", utils::datetime::kDefaultFormat),
               utils::datetime::DateParseError);
}

USERVER_NAMESPACE_END
