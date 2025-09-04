#include <userver/ugrpc/datetime_utils.hpp>

#include <chrono>
#include <cstdint>

#include <date/date.h>
#include <fmt/chrono.h>
#include <fmt/format.h>

#include <userver/formats/json/value_builder.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/datetime/cpp_20_calendar.hpp>
#include <userver/utils/datetime_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

namespace {

using Seconds = std::chrono::seconds;
using Nanos = std::chrono::nanoseconds;

constexpr utils::datetime::Days kMinSeconds{utils::datetime::DaysBetweenYears(1970, 1)};
constexpr utils::datetime::Days kMaxSeconds{utils::datetime::DaysBetweenYears(1970, 10000)};
constexpr Seconds kDurationSecondsLimit{static_cast<std::int64_t>(10000 * 365.25 * utils::datetime::Days::period::num)};

}  // namespace

bool IsValid(const google::protobuf::Timestamp& grpc_ts) {
    const Seconds seconds(grpc_ts.seconds());
    const Nanos nanos(grpc_ts.nanos());
    return (seconds >= kMinSeconds && seconds < kMaxSeconds) && (nanos >= Seconds(0) && nanos < Seconds(1));
}

google::protobuf::Timestamp NowTimestamp() { return ToProtoTimestamp(utils::datetime::Now()); }

bool IsValid(const google::type::Date& grpc_date) {
    return (grpc_date.year() >= 1 && grpc_date.year() < 10'000) &&
           (grpc_date.month() >= 1 && grpc_date.month() <= 12) &&
           (grpc_date.day() >= 1 &&
            date::day(grpc_date.day()) <= utils::datetime::DaysInMonth(grpc_date.month(), grpc_date.year()));
}

#if __cpp_lib_chrono >= 201907L

google::type::Date ToProtoDate(const std::chrono::year_month_day& system_date) {
    google::type::Date date;
    date.set_year(static_cast<int>(system_date.year()));
    date.set_month(static_cast<std::uint32_t>(system_date.month()));
    date.set_day(static_cast<std::uint32_t>(system_date.day()));
    if (!IsValid(date)) {
        throw DateConversionError("system_date is invalid");
    }
    return date;
}

std::chrono::year_month_day ToYearMonthDay(const google::type::Date& grpc_date) {
    if (!IsValid(grpc_date)) {
        throw DateConversionError("grpc_date is invalid");
    }
    return {
        std::chrono::year(grpc_date.year()), std::chrono::month(grpc_date.month()), std::chrono::day(grpc_date.day())};
}

#endif

google::type::Date ToProtoDate(const utils::datetime::Date& utils_date) {
    const date::year_month_day ymd(utils_date.GetSysDays());

    google::type::Date date;
    date.set_year(static_cast<int>(ymd.year()));
    date.set_month(static_cast<std::uint32_t>(ymd.month()));
    date.set_day(static_cast<std::uint32_t>(ymd.day()));
    if (!IsValid(date)) {
        throw DateConversionError("utils_date is invalid");
    }
    return date;
}

utils::datetime::Date ToUtilsDate(const google::type::Date& grpc_date) {
    if (!IsValid(grpc_date)) {
        throw DateConversionError("grpc_date is invalid");
    }
    return utils::datetime::Date(grpc_date.year(), grpc_date.month(), grpc_date.day());
}

std::chrono::time_point<std::chrono::system_clock, utils::datetime::Days> ToTimePoint(
    const google::type::Date& grpc_date
) {
    return ugrpc::ToUtilsDate(grpc_date).GetSysDays();
}

google::type::Date NowDate() { return ToProtoDate(utils::datetime::Now()); }

bool IsValid(const google::protobuf::Duration& grpc_duration) {
    const Seconds seconds(grpc_duration.seconds());
    const Nanos nanos(grpc_duration.nanos());
    return (seconds >= -kDurationSecondsLimit && seconds <= kDurationSecondsLimit) &&
           (nanos > Seconds(-1) && nanos < Seconds(1)) &&
           (seconds == Seconds(0) || nanos == Nanos(0) || (seconds < Seconds(0)) == (nanos < Nanos(0)));
}

}  // namespace ugrpc

namespace formats::parse {

google::protobuf::Timestamp Parse(const formats::json::Value& json, formats::parse::To<google::protobuf::Timestamp>) {
    return ugrpc::ToProtoTimestamp(
        json.As<std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>>()
    );
}

google::type::Date Parse(const formats::json::Value& json, formats::parse::To<google::type::Date>) {
    return ugrpc::ToProtoDate(json.As<utils::datetime::Date>());
}

}  // namespace formats::parse

namespace formats::serialize {

formats::json::Value Serialize(const google::protobuf::Timestamp& value, formats::serialize::To<formats::json::Value>) {
    if (!ugrpc::IsValid(value)) {
        throw ugrpc::TimestampConversionError("grpc_ts is invalid");
    }

    const auto seconds =
        std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>{std::chrono::seconds{value.seconds()}};
    std::string result = fmt::format("{:%FT%T}.{:0>9}", fmt::gmtime(seconds.time_since_epoch().count()), value.nanos());

    while (result.back() == '0') {
        result.pop_back();
    }
    if (result.back() == '.') {
        result.pop_back();
    }
    result.push_back('Z');
    return formats::json::ValueBuilder(result).ExtractValue();
}

formats::json::Value Serialize(const google::type::Date& value, formats::serialize::To<formats::json::Value>) {
    return formats::json::ValueBuilder(ugrpc::ToUtilsDate(value)).ExtractValue();
}

}  // namespace formats::serialize

USERVER_NAMESPACE_END
