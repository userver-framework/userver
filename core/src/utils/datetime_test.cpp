#include <gtest/gtest.h>

#include <userver/utils/datetime.hpp>
#include <userver/utils/datetime/date.hpp>
#include <userver/utils/datetime/from_string_saturating.hpp>

#include <cctz/time_zone.h>

USERVER_NAMESPACE_BEGIN

namespace {

TEST(Datetime, Timestring) {
    /// [Timestring example]
    const auto format = "%Y-%m-%dT%H:%M:%E*S%z";
    const auto time_string = utils::datetime::Timestring(std::time_t{0}, "Europe/Moscow", format);  // UTC+3 zone
    EXPECT_EQ(time_string, "1970-01-01T03:00:00+0300");
    /// [Timestring example]
}

TEST(Datetime, UtcStringtime) {
    /// [Stringtime example]
    const auto tp = utils::datetime::UtcStringtime("2014-03-17T02:47:07+0000");
    EXPECT_EQ(utils::datetime::UtcStringtime("2014-03-17T02:47:07+0000"), tp);
    /// [Stringtime example]
}

TEST(Datetime, OptionalStringtime) {
    /// [OptionalStringtime example]
    const auto tp = utils::datetime::UtcStringtime("2014-03-17T02:47:07+0000");
    EXPECT_EQ(utils::datetime::OptionalStringtime("2014-03-17T02:47:07+0000").value(), tp);
    /// [OptionalStringtime example]
}

TEST(Datetime, UtcTimestring) {
    /// [UtcTimestring example]
    const auto tp = utils::datetime::UtcStringtime("2014-03-17T02:47:07+0000");
    EXPECT_EQ(utils::datetime::UtcTimestring(tp), "2014-03-17T02:47:07+0000");
    /// [UtcTimestring example]
}

TEST(Datetime, UtcTimestringDate) {
    const auto tp = utils::datetime::Date(2025, 11, 15).GetSysDays();
    EXPECT_EQ(utils::datetime::UtcTimestring<utils::datetime::Days>(tp), "2025-11-15T00:00:00+0000");
}

TEST(Datetime, UtcTimestringCTime) {
    /// [UtcTimestring C time example]
    const auto tp = utils::datetime::UtcStringtime("2014-03-17T02:47:07+0000");
    const auto c_time = utils::datetime::Timestamp(tp);
    EXPECT_EQ(utils::datetime::UtcTimestring(c_time), "2014-03-17T02:47:07+0000");
    EXPECT_EQ(utils::datetime::TimestampToString(c_time), "2014-03-17T02:47:07+0000");
    /// [UtcTimestring C time example]
}

TEST(Datetime, GuessStringtime) {
    /// [GuessStringtime example]
    const auto tp = utils::datetime::UtcStringtime("2014-03-17T02:47:07+0000");
    EXPECT_EQ(utils::datetime::GuessStringtime("2014-03-17T02:47:07+0000", "UTC"), tp);
    /// [GuessStringtime example]
}

TEST(Datetime, Localize) {
    /// [Localize example]
    const auto tp = utils::datetime::UtcStringtime("2014-03-17T02:47:07+0000");
    const auto localize = utils::datetime::Localize(tp, utils::datetime::kDefaultDriverTimezone);
    const auto unlocalize = utils::datetime::Unlocalize(localize, utils::datetime::kDefaultDriverTimezone);
    EXPECT_EQ(utils::datetime::TimestampToString(unlocalize), "2014-03-17T02:47:07+0000");
    /// [Localize example]
}

TEST(Datetime, TimePointToTicks) {
    /// [TimePointToTicks example]
    const auto tp = utils::datetime::FromStringSaturating("2014-03-17 02:47:07.333304", "%Y-%m-%d %H:%M:%E*S");
    const auto ticks = utils::datetime::TimePointToTicks(tp);
    EXPECT_EQ((int64_t)635306212273333040, ticks);
    EXPECT_EQ(utils::datetime::TicksToTimePoint(ticks), tp);
    /// [TimePointToTicks example]
}

TEST(Datetime, kDefaultDriverTimezone) {
    /// [kDefaultDriverTimezone]
    const auto tp = utils::datetime::Stringtime(
        "2014-03-17T02:47:07+00:00", utils::datetime::kDefaultDriverTimezone, utils::datetime::kRfc3339Format
    );
    EXPECT_EQ(utils::datetime::Stringtime("2014-03-17T02:47:07+0000"), tp);
    /// [kDefaultDriverTimezone]
}

TEST(Datetime, kDefaultTimezone) {
    /// [kDefaultTimezone]
    const auto tp = utils::datetime::UtcStringtime("2014-03-17T02:47:07+00:00", utils::datetime::kRfc3339Format);
    EXPECT_EQ(utils::datetime::UtcStringtime("2014-03-17T02:47:07+0000"), tp);
    /// [kDefaultTimezone]
}

TEST(Datetime, kIncorrectDatetimeFormat) {
    /// [kIncorrectDatetimeFormat]
    EXPECT_FALSE(utils::datetime::OptionalStringtime("20140317T02:47:07+0000").has_value());
    /// [kIncorrectDatetimeFormat]
}

TEST(Datetime, kEmptyDatetime) {
    /// [kEmptyDatetime]
    EXPECT_FALSE(utils::datetime::OptionalStringtime("").has_value());
    /// [kEmptyDatetime]
}

}  // namespace

USERVER_NAMESPACE_END
