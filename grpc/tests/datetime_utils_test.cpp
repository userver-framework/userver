#include <userver/ugrpc/datetime_utils.hpp>

#include <chrono>
#include <cstdint>
#include <ratio>
#include <version>

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/util/message_differencer.h>
#include <google/protobuf/util/time_util.h>
#include <google/type/date.pb.h>
#include <gtest/gtest.h>

#include <userver/formats/json/value_builder.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utils/datetime/cpp_20_calendar.hpp>
#include <userver/utils/mock_now.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr auto kGrpcCompare = google::protobuf::util::MessageDifferencer::Equals;

google::protobuf::Timestamp MakeTimestamp(std::int64_t seconds, std::int32_t nanos) {
    google::protobuf::Timestamp ts;
    ts.set_seconds(seconds);
    ts.set_nanos(nanos);
    return ts;
}

constexpr utils::datetime::Days kManySeconds = utils::datetime::DaysBetweenYears(0, 11000);

const google::protobuf::Timestamp kTimestamp = MakeTimestamp(5, 123000);
constexpr std::chrono::system_clock::time_point kTimePoint(std::chrono::seconds(5) + std::chrono::microseconds(123));
const formats::json::Value kTsJson = formats::json::ValueBuilder("1970-01-01T00:00:05.000123Z").ExtractValue();

const google::protobuf::Timestamp kBigGrpcTimestamp = MakeTimestamp(1e11, 234567000);
constexpr std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> kBigTimestampMilliseconds(
    std::chrono::seconds(static_cast<std::int64_t>(1e11)) + std::chrono::milliseconds(234)
);
const formats::json::Value kBigTsJson = formats::json::ValueBuilder("5138-11-16T09:46:40.234567Z").ExtractValue();

}  // namespace

TEST(DatetimeUtilsTimestampIsValid, Valid) { EXPECT_TRUE(ugrpc::IsValid(kTimestamp)); }
TEST(DatetimeUtilsTimestampIsValid, BigSeconds) { EXPECT_FALSE(ugrpc::IsValid(MakeTimestamp(1e15, 0))); }
TEST(DatetimeUtilsTimestampIsValid, LowSeconds) { EXPECT_FALSE(ugrpc::IsValid(MakeTimestamp(-1e15, 0))); }
TEST(DatetimeUtilsTimestampIsValid, NegativeNanos) { EXPECT_FALSE(ugrpc::IsValid(MakeTimestamp(100, -5))); }
TEST(DatetimeUtilsTimestampIsValid, BigNanos) { EXPECT_FALSE(ugrpc::IsValid(MakeTimestamp(100, 1'000'000'000))); }

TEST(DatetimeUtils, SystemTimePointToProtoTimestamp) {
    EXPECT_TRUE(kGrpcCompare(kTimestamp, ugrpc::ToProtoTimestamp(kTimePoint)));
}

TEST(DatetimeUtils, InvalidTimePointToProtoTimestamp) {
    std::chrono::time_point<std::chrono::system_clock, utils::datetime::Days> system_ts(kManySeconds);
    UEXPECT_THROW_MSG(ugrpc::ToProtoTimestamp(system_ts), ugrpc::TimestampConversionError, "system_tp is invalid");
}

TEST(DatetimeUtils, TimestampToSystemClock) {
    EXPECT_EQ(kTimePoint, ugrpc::ToTimePoint<std::chrono::nanoseconds>(kTimestamp));
}

TEST(DatetimeUtils, BigTimestampToMilliseconds) {
    EXPECT_EQ(kBigTimestampMilliseconds, ugrpc::ToTimePoint<std::chrono::milliseconds>(kBigGrpcTimestamp));
}

TEST(DatetimeUtils, BigTimestampToNanoseconds) {
    UEXPECT_THROW_MSG(
        ugrpc::ToTimePoint<std::chrono::nanoseconds>(kBigGrpcTimestamp),
        ugrpc::TimestampConversionError,
        "grpc_ts does not fit the output type"
    );
}

TEST(DatetimeUtils, NegativeBigTimestampToNanoseconds) {
    const google::protobuf::Timestamp negative_big = MakeTimestamp(-10'000'000'000, 0);
    ASSERT_TRUE(ugrpc::IsValid(negative_big));
    UEXPECT_THROW_MSG(
        ugrpc::ToTimePoint<std::chrono::nanoseconds>(negative_big),
        ugrpc::TimestampConversionError,
        "grpc_ts does not fit the output type"
    );
}

TEST(DatetimeUtils, TimestampToNanosecondsLastWholeSecond) {
    constexpr auto kTimestampString = "2262-04-11T23:47:16Z";
    constexpr std::chrono::nanoseconds kExpectedDuration{9'223'372'036'000'000'000};
    google::protobuf::Timestamp timestamp;
    ASSERT_TRUE(google::protobuf::util::TimeUtil::FromString(kTimestampString, &timestamp));

    EXPECT_EQ(kExpectedDuration, ugrpc::ToTimePoint<std::chrono::nanoseconds>(timestamp).time_since_epoch());
}

TEST(DatetimeUtils, TimestampToNanosecondsLimits) {
    using Duration = std::chrono::nanoseconds;
    const auto min_timestamp = MakeTimestamp(-9'223'372'037, 145'224'192);
    const auto max_timestamp = MakeTimestamp(9'223'372'036, 854'775'807);

    EXPECT_EQ(Duration::min(), ugrpc::ToTimePoint<Duration>(min_timestamp).time_since_epoch());
    EXPECT_EQ(Duration::max(), ugrpc::ToTimePoint<Duration>(max_timestamp).time_since_epoch());

    auto below_min = min_timestamp;
    below_min.set_nanos(below_min.nanos() - 1);
    UEXPECT_THROW_MSG(
        ugrpc::ToTimePoint<Duration>(below_min),
        ugrpc::TimestampConversionError,
        "grpc_ts does not fit the output type"
    );

    auto above_max = max_timestamp;
    above_max.set_nanos(above_max.nanos() + 1);
    UEXPECT_THROW_MSG(
        ugrpc::ToTimePoint<Duration>(above_max),
        ugrpc::TimestampConversionError,
        "grpc_ts does not fit the output type"
    );
}

TEST(DatetimeUtils, TimestampToUnsignedNanoseconds) {
    using Duration = std::chrono::duration<std::uint64_t, std::nano>;
    const auto max_timestamp = MakeTimestamp(18'446'744'073, 709'551'615);

    EXPECT_EQ(Duration::zero(), ugrpc::ToTimePoint<Duration>(MakeTimestamp(0, 0)).time_since_epoch());
    EXPECT_EQ(Duration::max(), ugrpc::ToTimePoint<Duration>(max_timestamp).time_since_epoch());
    UEXPECT_THROW_MSG(
        ugrpc::ToTimePoint<Duration>(MakeTimestamp(-1, 999'999'999)),
        ugrpc::TimestampConversionError,
        "grpc_ts does not fit the output type"
    );

    auto above_max = max_timestamp;
    above_max.set_nanos(above_max.nanos() + 1);
    UEXPECT_THROW_MSG(
        ugrpc::ToTimePoint<Duration>(above_max),
        ugrpc::TimestampConversionError,
        "grpc_ts does not fit the output type"
    );
}

TEST(DatetimeUtils, TimestampToMicrosecondsTruncatesNanos) {
    EXPECT_EQ(
        std::chrono::microseconds(-1),
        ugrpc::ToTimePoint<std::chrono::microseconds>(MakeTimestamp(-1, 999'999'999)).time_since_epoch()
    );
}

TEST(DatetimeUtils, InvalidTimestampToSystemClock) {
    UEXPECT_THROW_MSG(
        ugrpc::ToTimePoint(MakeTimestamp(1e15, 0)),
        ugrpc::TimestampConversionError,
        "grpc_ts is invalid"
    );
}

TEST(DatetimeUtils, NowTimestamp) {
    utils::datetime::MockNowSet(kTimePoint);
    EXPECT_TRUE(kGrpcCompare(kTimestamp, ugrpc::NowTimestamp()));
}

TEST(DatetimeUtils, TimestampJsonParse) {
    EXPECT_TRUE(kGrpcCompare(kTimestamp, kTsJson.As<google::protobuf::Timestamp>()));
}

TEST(DatetimeUtils, BigTimestampJsonParse) {
    EXPECT_TRUE(kGrpcCompare(kBigGrpcTimestamp, kBigTsJson.As<google::protobuf::Timestamp>()));
}

TEST(DatetimeUtils, TimestampJsonSerialize) {
    EXPECT_EQ(formats::json::ValueBuilder(kTimestamp).ExtractValue(), kTsJson);
}

TEST(DatetimeUtils, TimestampJsonSerializeNoNanos) {
    EXPECT_EQ(
        formats::json::ValueBuilder(MakeTimestamp(5, 0)).ExtractValue(),
        formats::json::ValueBuilder("1970-01-01T00:00:05Z").ExtractValue()
    );
}

TEST(DatetimeUtils, BigTimestampJsonSerialize) {
    EXPECT_EQ(formats::json::ValueBuilder(kBigGrpcTimestamp).ExtractValue(), kBigTsJson);
}

TEST(DatetimeUtils, InvalidTimestampJsonSerialize) {
    UEXPECT_THROW_MSG(
        formats::json::ValueBuilder(MakeTimestamp(1e15, 0)).ExtractValue(),
        ugrpc::TimestampConversionError,
        "grpc_ts is invalid"
    );
}

namespace {

#if __cpp_lib_chrono >= 201907L
constexpr std::chrono::year_month_day kYearMonthDay(
    std::chrono::year(2025),
    std::chrono::month(4),
    std::chrono::day(10)
);
#endif

google::type::Date MakeDate(std::int32_t year, std::int32_t month, std::int32_t day) {
    google::type::Date date;
    date.set_year(year);
    date.set_month(month);
    date.set_day(day);
    return date;
}

const google::type::Date kDate = MakeDate(2025, 4, 10);
const utils::datetime::Date kUtilsDate(2025, 4, 10);
constexpr std::chrono::system_clock::time_point kDateTimePoint(std::chrono::seconds(1744287725));
constexpr std::chrono::system_clock::time_point kDateTimePointRounded(std::chrono::seconds(1744243200));
const formats::json::Value kDateJson = formats::json::ValueBuilder("2025-04-10").ExtractValue();

}  // namespace

TEST(DatetimeUtilsDateIsValid, Valid) { EXPECT_TRUE(ugrpc::IsValid(kDate)); }
TEST(DatetimeUtilsDateIsValid, NegativeYear) { EXPECT_FALSE(ugrpc::IsValid(MakeDate(-10, 4, 10))); }
TEST(DatetimeUtilsDateIsValid, BigYear) { EXPECT_FALSE(ugrpc::IsValid(MakeDate(11000, 4, 10))); }
TEST(DatetimeUtilsDateIsValid, NegativeMonth) { EXPECT_FALSE(ugrpc::IsValid(MakeDate(2025, -5, 10))); }
TEST(DatetimeUtilsDateIsValid, BigMonth) { EXPECT_FALSE(ugrpc::IsValid(MakeDate(2025, 15, 10))); }
TEST(DatetimeUtilsDateIsValid, NegativeDay) { EXPECT_FALSE(ugrpc::IsValid(MakeDate(2025, 4, -10))); }
TEST(DatetimeUtilsDateIsValid, BigDay) { EXPECT_FALSE(ugrpc::IsValid(MakeDate(2025, 4, 50))); }
TEST(DatetimeUtilsDateIsValid, DayDoesNotMatchMonth) { EXPECT_FALSE(ugrpc::IsValid(MakeDate(2025, 4, 31))); }
// day is a wire-controlled int32; values above 255 must not wrap into the valid range
TEST(DatetimeUtilsDateIsValid, DayWrapsToZero) { EXPECT_FALSE(ugrpc::IsValid(MakeDate(2025, 4, 256))); }
TEST(DatetimeUtilsDateIsValid, DayWrapsIntoValidRange) { EXPECT_FALSE(ugrpc::IsValid(MakeDate(2025, 4, 286))); }

#if __cpp_lib_chrono >= 201907L

TEST(DatetimeUtils, ToProtoDateFromYearMonthDay) {
    EXPECT_TRUE(kGrpcCompare(kDate, ugrpc::ToProtoDate(kYearMonthDay)));
}

// TODO reenable when formats:: supports serialization for std::year_month_day.
#if 0

TEST(DatetimeUtils, ToProtoDateFromInvalidYearMonthDay) {
    constexpr std::chrono::year_month_day
        kInvalidYearMonthDay(std::chrono::year(11000), std::chrono::month(4), std::chrono::day(10));
    UEXPECT_THROW_MSG(
        formats::json::ValueBuilder(kInvalidYearMonthDay).ExtractValue(),
        ugrpc::DateConversionError,
        "system_date is invalid"
    );
}

#endif

TEST(DatetimeUtils, ToYearMonthDay) { EXPECT_EQ(kYearMonthDay, ugrpc::ToYearMonthDay(kDate)); }

TEST(DatetimeUtils, ToYearMonthDayInvalidDate) {
    UEXPECT_THROW_MSG(
        ugrpc::ToYearMonthDay(MakeDate(11000, 4, 10)),
        ugrpc::DateConversionError,
        "grpc_date is invalid"
    );
}

#endif

TEST(DatetimeUtils, ToProtoDateFromUtilsDate) { EXPECT_TRUE(kGrpcCompare(kDate, ugrpc::ToProtoDate(kUtilsDate))); }

TEST(DatetimeUtils, ToProtoDateFromInvalidUtilsDate) {
    UEXPECT_THROW_MSG(
        ugrpc::ToProtoDate(utils::datetime::Date(11000, 4, 10)),
        ugrpc::DateConversionError,
        "utils_date is invalid"
    );
}

TEST(DatetimeUtils, ToUtilsDate) { EXPECT_EQ(kUtilsDate, ugrpc::ToUtilsDate(kDate)); }

TEST(DatetimeUtils, ToUtilsDateInvalid) {
    UEXPECT_THROW_MSG(ugrpc::ToUtilsDate(MakeDate(11000, 4, 10)), ugrpc::DateConversionError, "grpc_date is invalid");
}

TEST(DatetimeUtils, ToProtoDateFromTimePoint) { EXPECT_TRUE(kGrpcCompare(kDate, ugrpc::ToProtoDate(kDateTimePoint))); }

TEST(DatetimeUtils, ToProtoDateFromInvalidTimePoint) {
    std::chrono::time_point<std::chrono::system_clock, utils::datetime::Days> system_ts(kManySeconds);
    UEXPECT_THROW_MSG(ugrpc::ToProtoDate(system_ts), ugrpc::DateConversionError, "utils_date is invalid");
}

TEST(DatetimeUtils, DateToSystemClock) { EXPECT_EQ(kDateTimePointRounded, ugrpc::ToTimePoint(kDate)); }

TEST(DatetimeUtils, InvalidDateToSystemClock) {
    UEXPECT_THROW_MSG(ugrpc::ToTimePoint(MakeDate(11000, 4, 10)), ugrpc::DateConversionError, "grpc_date is invalid");
}

TEST(DatetimeUtils, NowDate) {
    utils::datetime::MockNowSet(kDateTimePoint);
    EXPECT_TRUE(kGrpcCompare(kDate, ugrpc::NowDate()));
}

TEST(DatetimeUtils, DateJsonParse) { EXPECT_TRUE(kGrpcCompare(kDate, kDateJson.As<google::type::Date>())); }

TEST(DatetimeUtils, DateJsonSerialize) { EXPECT_EQ(formats::json::ValueBuilder(kDate).ExtractValue(), kDateJson); }

TEST(DatetimeUtils, InvalidDateJsonSerialize) {
    UEXPECT_THROW_MSG(
        formats::json::ValueBuilder(MakeDate(11000, 4, 10)).ExtractValue(),
        ugrpc::DateConversionError,
        "grpc_date is invalid"
    );
}

namespace {

google::protobuf::Duration MakeDuration(std::int64_t seconds, std::int32_t nanos) {
    google::protobuf::Duration duration;
    duration.set_seconds(seconds);
    duration.set_nanos(nanos);
    return duration;
}

const google::protobuf::Duration kGrpcDuration = MakeDuration(123, 5678);
constexpr std::chrono::duration kDuration = std::chrono::seconds(123) + std::chrono::nanoseconds(5678);

const google::protobuf::Duration kBigGrpcDuration = MakeDuration(1e11, 234567000);
constexpr std::chrono::duration kBigDurationMicroseconds =
    std::chrono::seconds(static_cast<std::int64_t>(1e11)) + std::chrono::microseconds(234567);
constexpr std::chrono::duration
    kBigDurationMilliseconds = std::chrono::seconds(static_cast<std::int64_t>(1e11)) + std::chrono::milliseconds(234);

}  // namespace

TEST(DatetimeUtilsDurationIsValid, Valid) { EXPECT_TRUE(ugrpc::IsValid(kGrpcDuration)); }
TEST(DatetimeUtilsDurationIsValid, BigSeconds) { EXPECT_FALSE(ugrpc::IsValid(MakeDuration(1e15, 0))); }
TEST(DatetimeUtilsDurationIsValid, LowSeconds) { EXPECT_FALSE(ugrpc::IsValid(MakeDuration(-1e15, 0))); }
TEST(DatetimeUtilsDurationIsValid, BigNanos) { EXPECT_FALSE(ugrpc::IsValid(MakeDuration(0, 1e9))); }
TEST(DatetimeUtilsDurationIsValid, LowNanos) { EXPECT_FALSE(ugrpc::IsValid(MakeDuration(0, -1e9))); }
TEST(DatetimeUtilsDurationIsValid, SignMismatchNegPos) { EXPECT_FALSE(ugrpc::IsValid(MakeDuration(10, -5))); }
TEST(DatetimeUtilsDurationIsValid, SignMismatchPosNeg) { EXPECT_FALSE(ugrpc::IsValid(MakeDuration(-10, 5))); }

TEST(DatetimeUtils, ToDuration) { EXPECT_EQ(kDuration, ugrpc::ToDuration<std::chrono::nanoseconds>(kGrpcDuration)); }

TEST(DatetimeUtils, BigDurationToMicroseconds) {
    EXPECT_EQ(kBigDurationMicroseconds, ugrpc::ToDuration(kBigGrpcDuration));
}

TEST(DatetimeUtils, BigDurationToMilliseconds) {
    EXPECT_EQ(kBigDurationMilliseconds, ugrpc::ToDuration<std::chrono::milliseconds>(kBigGrpcDuration));
}

TEST(DatetimeUtils, BigDurationToNanoseconds) {
    UEXPECT_THROW_MSG(
        ugrpc::ToDuration<std::chrono::nanoseconds>(kBigGrpcDuration),
        ugrpc::DurationConversionError,
        "grpc_duration does not fit the output type"
    );
}

TEST(DatetimeUtils, NegativeBigDurationToNanoseconds) {
    const google::protobuf::Duration negative_big = MakeDuration(-100'000'000'000, 0);
    ASSERT_TRUE(ugrpc::IsValid(negative_big));
    UEXPECT_THROW_MSG(
        ugrpc::ToDuration<std::chrono::nanoseconds>(negative_big),
        ugrpc::DurationConversionError,
        "grpc_duration does not fit the output type"
    );
}

TEST(DatetimeUtils, DurationToNanosecondsLimits) {
    using Duration = std::chrono::nanoseconds;
    const auto min_duration = MakeDuration(-9'223'372'036, -854'775'808);
    const auto max_duration = MakeDuration(9'223'372'036, 854'775'807);

    EXPECT_EQ(Duration::min(), ugrpc::ToDuration<Duration>(min_duration));
    EXPECT_EQ(Duration::max(), ugrpc::ToDuration<Duration>(max_duration));

    auto below_min = min_duration;
    below_min.set_nanos(below_min.nanos() - 1);
    UEXPECT_THROW_MSG(
        ugrpc::ToDuration<Duration>(below_min),
        ugrpc::DurationConversionError,
        "grpc_duration does not fit the output type"
    );

    auto above_max = max_duration;
    above_max.set_nanos(above_max.nanos() + 1);
    UEXPECT_THROW_MSG(
        ugrpc::ToDuration<Duration>(above_max),
        ugrpc::DurationConversionError,
        "grpc_duration does not fit the output type"
    );
}

TEST(DatetimeUtils, DurationToUnsignedNanoseconds) {
    using Duration = std::chrono::duration<std::uint64_t, std::nano>;
    const auto max_duration = MakeDuration(18'446'744'073, 709'551'615);

    EXPECT_EQ(Duration::zero(), ugrpc::ToDuration<Duration>(MakeDuration(0, 0)));
    EXPECT_EQ(Duration::max(), ugrpc::ToDuration<Duration>(max_duration));
    UEXPECT_THROW_MSG(
        ugrpc::ToDuration<Duration>(MakeDuration(0, -1)),
        ugrpc::DurationConversionError,
        "grpc_duration does not fit the output type"
    );

    auto above_max = max_duration;
    above_max.set_nanos(above_max.nanos() + 1);
    UEXPECT_THROW_MSG(
        ugrpc::ToDuration<Duration>(above_max),
        ugrpc::DurationConversionError,
        "grpc_duration does not fit the output type"
    );
}

TEST(DatetimeUtils, DurationToSecondsLimits) {
    using Duration = std::chrono::duration<std::int32_t>;
    const auto min_duration = MakeDuration(Duration::min().count(), -999'999'999);
    const auto max_duration = MakeDuration(Duration::max().count(), 999'999'999);

    EXPECT_EQ(Duration::min(), ugrpc::ToDuration<Duration>(min_duration));
    EXPECT_EQ(Duration::max(), ugrpc::ToDuration<Duration>(max_duration));
}

TEST(DatetimeUtils, DurationToFloatingSeconds) {
    using Duration = std::chrono::duration<double>;

    EXPECT_DOUBLE_EQ(-1.5, ugrpc::ToDuration<Duration>(MakeDuration(-1, -500'000'000)).count());
}

TEST(DatetimeUtils, ToDurationInvalid) {
    UEXPECT_THROW_MSG(
        ugrpc::ToDuration(MakeDuration(10, -5)),
        ugrpc::DurationConversionError,
        "grpc_duration is invalid"
    );
}

TEST(DatetimeUtils, ToProtoDuration) { EXPECT_TRUE(kGrpcCompare(kGrpcDuration, ugrpc::ToProtoDuration(kDuration))); }

TEST(DatetimeUtils, ToProtoDurationInvalid) {
    UEXPECT_THROW_MSG(ugrpc::ToProtoDuration(kManySeconds), ugrpc::DurationConversionError, "duration is invalid");
}

USERVER_NAMESPACE_END
