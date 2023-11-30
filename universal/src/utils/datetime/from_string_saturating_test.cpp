#include <userver/utils/datetime/from_string_saturating.hpp>

#include <gtest/gtest.h>

#include <userver/utils/datetime.hpp>
#include <userver/utils/mock_now.hpp>

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
  /// [FromStringSaturation example]
  EXPECT_EQ(
      utils::datetime::FromRfc3339StringSaturating("10000-01-01T00:00:00+0000"),
      std::chrono::system_clock::time_point::max());
  /// [FromStringSaturation example]
}

TEST(FromStringSaturation, MicroSeconds) {
  constexpr auto format = "%Y-%m-%d %H:%M:%E*S";
  utils::datetime::MockNowSet(utils::datetime::FromStringSaturating(
      "1970-01-01 00:00:00.000000", format));

  auto tp =
      utils::datetime::Now() + std::chrono::microseconds(1395024427333304);
  EXPECT_EQ(utils::datetime::FromStringSaturating("2014-03-17 02:47:07.333304",
                                                  format),
            tp);

  tp += std::chrono::microseconds(1);

  EXPECT_EQ(utils::datetime::FromStringSaturating("2014-03-17 02:47:07.333305",
                                                  format),
            tp);

  tp -= std::chrono::microseconds(6);

  EXPECT_EQ(utils::datetime::FromStringSaturating("2014-03-17 02:47:07.333299",
                                                  format),
            tp);
}

TEST(FromStringSaturation, NotSupported) {
  constexpr auto bad_format = "%Y-%m-%dT%H:%M:%S.%f";
  EXPECT_THROW(utils::datetime::FromStringSaturating(
                   "2023-06-06T15:23:19.231934", bad_format),
               utils::datetime::DateParseError);
}

TEST(FromStringSaturation, MicroSecondsAdditionalFormat) {
  auto tp1 = utils::datetime::FromStringSaturating("2014-03-17 02:47:07.333305",
                                                   "%Y-%m-%d %H:%M:%S.%E*f");
  auto tp2 = utils::datetime::FromStringSaturating("2014-03-17 02:47:07.333300",
                                                   "%Y-%m-%d %H:%M:%S.%E*f");

  EXPECT_NE(tp1, tp2);

  tp1 -= std::chrono::microseconds(3);
  tp2 += std::chrono::microseconds(2);

  EXPECT_EQ(tp1, tp2);
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

TEST(FromStringSaturation, kRfc3339Format) {
  /// [kRfc3339Format]
  const auto exp = utils::datetime::FromStringSaturating(
      "2014-03-17 02:47:07.000000", "%Y-%m-%d %H:%M:%E*S");
  const auto tp = utils::datetime::FromStringSaturating(
      "2014-03-17T02:47:07+00:00", utils::datetime::kRfc3339Format);
  EXPECT_EQ(tp, exp);
  /// [kRfc3339Format]
}

TEST(FromStringSaturation, kTaximeterFormat) {
  /// [kTaximeterFormat]
  const auto exp = utils::datetime::FromStringSaturating(
      "2014-03-17 02:47:07.000000", "%Y-%m-%d %H:%M:%E*S");
  const auto tp = utils::datetime::FromStringSaturating(
      "2014-03-17T02:47:07.000000Z", utils::datetime::kTaximeterFormat);
  EXPECT_EQ(tp, exp);
  /// [kTaximeterFormat]
}

TEST(FromStringSaturation, kDefaultFormat) {
  /// [kDefaultFormat]
  const auto exp = utils::datetime::FromStringSaturating(
      "2014-03-17 02:47:07.000000", "%Y-%m-%d %H:%M:%E*S");
  const auto tp = utils::datetime::FromStringSaturating(
      "2014-03-17T02:47:07.000000+0000", utils::datetime::kDefaultFormat);
  EXPECT_EQ(tp, exp);
  /// [kDefaultFormat]
}

TEST(FromStringSaturation, kIsoFormat) {
  /// [kIsoFormat]
  const auto exp = utils::datetime::FromStringSaturating(
      "2014-03-17 02:47:07.000000", "%Y-%m-%d %H:%M:%E*S");
  const auto tp = utils::datetime::FromStringSaturating(
      "2014-03-17T02:47:07Z", utils::datetime::kIsoFormat);
  EXPECT_EQ(tp, exp);
  /// [kIsoFormat]
}

USERVER_NAMESPACE_END
