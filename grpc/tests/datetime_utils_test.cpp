#include <userver/ugrpc/datetime_utils.hpp>

#include <chrono>
#include <version>

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/util/message_differencer.h>
#include <google/type/date.pb.h>
#include <gtest/gtest.h>

#include <userver/formats/json/value_builder.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utils/datetime/cpp_20_calendar.hpp>
#include <userver/utils/mock_now.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr auto GrpcCompare = google::protobuf::util::MessageDifferencer::Equals;

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
    EXPECT_TRUE(GrpcCompare(kTimestamp, ugrpc::ToProtoTimestamp(kTimePoint)));
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

TEST(DatetimeUtils, InvalidTimestampToSystemClock) {
    UEXPECT_THROW_MSG(
        ugrpc::ToTimePoint(MakeTimestamp(1e15, 0)), ugrpc::TimestampConversionError, "grpc_ts is invalid"
    );
}

TEST(DatetimeUtils, NowTimestamp) {
    utils::datetime::MockNowSet(kTimePoint);
    EXPECT_TRUE(GrpcCompare(kTimestamp, ugrpc::NowTimestamp()));
}

TEST(DatetimeUtils, TimestampJsonParse) {
    EXPECT_TRUE(GrpcCompare(kTimestamp, kTsJson.As<google::protobuf::Timestamp>()));
}

TEST(DatetimeUtils, BigTimestampJsonParse) {
    EXPECT_TRUE(GrpcCompare(kBigGrpcTimestamp, kBigTsJson.As<google::protobuf::Timestamp>()));
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
constexpr std::chrono::year_month_day
    kYearMonthDay(std::chrono::year(2025), std::chrono::month(4), std::chrono::day(10));
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

#if __cpp_lib_chrono >= 201907L

TEST(DatetimeUtils, ToProtoDateFromYearMonthDay) { EXPECT_TRUE(GrpcCompare(kDate, ugrpc::ToProtoDate(kYearMonthDay))); }

TEST(DatetimeUtils, ToProtoDateFromInvalidYearMonthDay) {
    constexpr std::chrono::year_month_day kInvalidYearMonthDay(
        std::chrono::year(11000), std::chrono::month(4), std::chrono::day(10)
    );
    UEXPECT_THROW_MSG(
        formats::json::ValueBuilder(kInvalidYearMonthDay).ExtractValue(),
        ugrpc::DateConversionError,
        "system_date is invalid"
    );
}

TEST(DatetimeUtils, ToYearMonthDay) { EXPECT_EQ(kYearMonthDay, ugrpc::ToYearMonthDay(kDate)); }

TEST(DatetimeUtils, ToYearMonthDayInvalidDate) {
    UEXPECT_THROW_MSG(
        ugrpc::ToYearMonthDay(MakeDate(11000, 4, 10)), ugrpc::DateConversionError, "grpc_date is invalid"
    );
}

#endif

TEST(DatetimeUtils, ToProtoDateFromUtilsDate) { EXPECT_TRUE(GrpcCompare(kDate, ugrpc::ToProtoDate(kUtilsDate))); }

TEST(DatetimeUtils, ToProtoDateFromInvalidUtilsDate) {
    UEXPECT_THROW_MSG(
        ugrpc::ToProtoDate(utils::datetime::Date(11000, 4, 10)), ugrpc::DateConversionError, "utils_date is invalid"
    );
}

TEST(DatetimeUtils, ToUtilsDate) { EXPECT_EQ(kUtilsDate, ugrpc::ToUtilsDate(kDate)); }

TEST(DatetimeUtils, ToUtilsDateInvalid) {
    UEXPECT_THROW_MSG(ugrpc::ToUtilsDate(MakeDate(11000, 4, 10)), ugrpc::DateConversionError, "grpc_date is invalid");
}

TEST(DatetimeUtils, ToProtoDateFromTimePoint) { EXPECT_TRUE(GrpcCompare(kDate, ugrpc::ToProtoDate(kDateTimePoint))); }

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
    EXPECT_TRUE(GrpcCompare(kDate, ugrpc::NowDate()));
}

TEST(DatetimeUtils, DateJsonParse) { EXPECT_TRUE(GrpcCompare(kDate, kDateJson.As<google::type::Date>())); }

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
constexpr std::chrono::duration kBigDurationMilliseconds =
    std::chrono::seconds(static_cast<std::int64_t>(1e11)) + std::chrono::milliseconds(234);

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

TEST(DatetimeUtils, ToDurationInvalid) {
    UEXPECT_THROW_MSG(
        ugrpc::ToDuration(MakeDuration(10, -5)), ugrpc::DurationConversionError, "grpc_duration is invalid"
    );
}

TEST(DatetimeUtils, ToProtoDuration) { EXPECT_TRUE(GrpcCompare(kGrpcDuration, ugrpc::ToProtoDuration(kDuration))); }

TEST(DatetimeUtils, ToProtoDurationInvalid) {
    UEXPECT_THROW_MSG(ugrpc::ToProtoDuration(kManySeconds), ugrpc::DurationConversionError, "duration is invalid");
}

USERVER_NAMESPACE_END
