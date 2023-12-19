#include <gtest/gtest.h>

#include <userver/utils/datetime.hpp>
#include <userver/utils/datetime/from_string_saturating.hpp>

#include <cctz/time_zone.h>

USERVER_NAMESPACE_BEGIN

namespace {

TEST(Datetime, Stringtime) {
  /// [Stringtime example]
  const auto tp = utils::datetime::Stringtime("2014-03-17T02:47:07+0000");
  EXPECT_EQ(utils::datetime::Stringtime("2014-03-17T02:47:07+0000"), tp);
  /// [Stringtime example]
}

TEST(Datetime, Timestring) {
  /// [Timestring example]
  const auto tp = utils::datetime::Stringtime("2014-03-17T02:47:07+0000");
  EXPECT_EQ(utils::datetime::Timestring(tp), "2014-03-17T02:47:07+0000");
  /// [Timestring example]
}

TEST(Datetime, TimestringCTime) {
  /// [Timestring C time example]
  const auto tp = utils::datetime::Stringtime("2014-03-17T02:47:07+0000");
  const auto c_time = utils::datetime::Timestamp(tp);
  EXPECT_EQ(utils::datetime::Timestring(c_time), "2014-03-17T02:47:07+0000");
  EXPECT_EQ(utils::datetime::TimestampToString(c_time),
            "2014-03-17T02:47:07+0000");
  /// [Timestring C time example]
}

TEST(Datetime, GuessStringtime) {
  /// [GuessStringtime example]
  const auto tp = utils::datetime::Stringtime("2014-03-17T02:47:07+0000");
  EXPECT_EQ(utils::datetime::GuessStringtime("2014-03-17T02:47:07+0000", "UTC"),
            tp);
  /// [GuessStringtime example]
}

TEST(Datetime, Localize) {
  /// [Localize example]
  const auto tp = utils::datetime::Stringtime("2014-03-17T02:47:07+0000");
  const auto localize =
      utils::datetime::Localize(tp, utils::datetime::kDefaultDriverTimezone);
  const auto unlocalize = utils::datetime::Unlocalize(
      localize, utils::datetime::kDefaultDriverTimezone);
  EXPECT_EQ(utils::datetime::TimestampToString(unlocalize),
            "2014-03-17T02:47:07+0000");
  /// [Localize example]
}

TEST(Datetime, TimePointToTicks) {
  /// [TimePointToTicks example]
  const auto tp = utils::datetime::FromStringSaturating(
      "2014-03-17 02:47:07.333304", "%Y-%m-%d %H:%M:%E*S");
  const auto ticks = utils::datetime::TimePointToTicks(tp);
  EXPECT_EQ((int64_t)635306212273333040, ticks);
  EXPECT_EQ(utils::datetime::TicksToTimePoint(ticks), tp);
  /// [TimePointToTicks example]
}

TEST(Datetime, kDefaultDriverTimezone) {
  /// [kDefaultDriverTimezone]
  const auto tp = utils::datetime::Stringtime(
      "2014-03-17T02:47:07+00:00", utils::datetime::kDefaultDriverTimezone,
      utils::datetime::kRfc3339Format);
  EXPECT_EQ(utils::datetime::Stringtime("2014-03-17T02:47:07+0000"), tp);
  /// [kDefaultDriverTimezone]
}

TEST(Datetime, kDefaultTimezone) {
  /// [kDefaultTimezone]
  const auto tp = utils::datetime::Stringtime("2014-03-17T02:47:07+00:00",
                                              utils::datetime::kDefaultTimezone,
                                              utils::datetime::kRfc3339Format);
  EXPECT_EQ(utils::datetime::Stringtime("2014-03-17T02:47:07+0000"), tp);
  /// [kDefaultTimezone]
}

}  // namespace

USERVER_NAMESPACE_END
