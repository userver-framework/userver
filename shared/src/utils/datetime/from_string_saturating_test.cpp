#include <utils/datetime/from_string_saturating.hpp>

#include <gtest/gtest.h>

namespace {

std::chrono::system_clock::time_point GetDateTimePlatformSpecificBigValue() {
#ifdef _LIBCPP_VERSION
  // MAC_COMPAT: std::chrono::system_clock::time_point in libc++ can handle
  // years greater than 9000. So the `From*StringSaturating` functions do not
  // saturate.
  return utils::datetime::FromRfc3339StringSaturating(
      "9000-12-31T00:00:00+0000");
#else
  return std::chrono::system_clock::time_point::max();
#endif
}

}  // namespace

TEST(FromStringSaturation, Rfc3339) {
  EXPECT_EQ(
      utils::datetime::FromRfc3339StringSaturating("9000-12-31T00:00:00+0000"),
      GetDateTimePlatformSpecificBigValue());

  EXPECT_EQ(
      utils::datetime::FromRfc3339StringSaturating("9999-12-31T00:00:00+0000"),
      std::chrono::system_clock::time_point::max());
}

TEST(FromStringSaturation, Formats) {
  EXPECT_EQ(utils::datetime::FromStringSaturating("9000-12-31T00:00:00Z",
                                                  "%Y-%m-%dT%H:%M:%SZ"),
            GetDateTimePlatformSpecificBigValue());

  EXPECT_EQ(utils::datetime::FromStringSaturating("9000-12-31", "%Y-%m-%d"),
            GetDateTimePlatformSpecificBigValue());

  EXPECT_EQ(utils::datetime::FromStringSaturating("9999-01-01", "%Y-%m-%d"),
            std::chrono::system_clock::time_point::max());
}
