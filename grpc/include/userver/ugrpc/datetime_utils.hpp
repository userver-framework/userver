#pragma once

/// @file
/// @brief Utilities for @c google::protobuf::Timestamp, @c google::type::Date, and @c google::protobuf::Duration types.

#include <chrono>
#include <stdexcept>
#include <version>

#include <google/protobuf/duration.pb.h>
#include <google/protobuf/timestamp.pb.h>
#include <google/type/date.pb.h>

#include <userver/formats/json_fwd.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/to.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/datetime/cpp_20_calendar.hpp>
#include <userver/utils/datetime/date.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

/// @brief Checks if @c google::protobuf::Timestamp contains a valid value according to protobuf documentation.
///
/// The timestamp must be from 0001-01-01T00:00:00Z to 9999-12-31T23:59:59Z inclusive
/// and 'nanos' must be from 0 to 999,999,999 inclusive.
bool IsValid(const google::protobuf::Timestamp& grpc_ts);

/// @brief Exception thrown when timestamp conversion functions receive/produce invalid @c google::protobuf::Timestamp
/// accrording to @c IsValid or otherwise lead to undefined behavior due to integer overflow.
class TimestampConversionError : public std::overflow_error {
    using std::overflow_error::overflow_error;
};

/// @brief Creates @c google::protobuf::Timestamp from @c std::chrono::time_point.
///
/// @throws TimestampConversionError if the value is not valid according to @c IsValid.
template <class Duration>
google::protobuf::Timestamp ToProtoTimestamp(
    const std::chrono::time_point<std::chrono::system_clock, Duration>& system_tp
) {
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(system_tp.time_since_epoch());
    const auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(system_tp.time_since_epoch() - seconds);

    google::protobuf::Timestamp timestamp;
    timestamp.set_seconds(seconds.count());
    timestamp.set_nanos(nanos.count());

    if (!IsValid(timestamp)) {
        throw TimestampConversionError("system_tp is invalid");
    }
    return timestamp;
}

/// @brief Creates @c std::chrono::system_clock::time_point from @c google::protobuf::Timestamp.
///
/// @throws TimestampConversionError if the value is not valid according to @c IsValid or does not fit the time_point.
template <class Duration = std::chrono::system_clock::duration>
std::chrono::time_point<std::chrono::system_clock, Duration> ToTimePoint(const google::protobuf::Timestamp& grpc_ts) {
    if (!IsValid(grpc_ts)) {
        throw TimestampConversionError("grpc_ts is invalid");
    }

    const std::chrono::seconds seconds(grpc_ts.seconds());
    // Equality is important here. Otherwise, grpc_duration.seconds() == std::chrono::seconds::max()
    // will lead to overflow if grpc_duration.nanos() != 0.
    if (seconds >= std::chrono::duration_cast<std::chrono::seconds>(Duration::max())) {
        throw TimestampConversionError("grpc_ts does not fit the output type");
    }

    return std::chrono::time_point<std::chrono::system_clock, Duration>(std::chrono::duration_cast<Duration>(
        seconds + std::chrono::duration_cast<Duration>(std::chrono::nanoseconds(grpc_ts.nanos()))
    ));
}

/// @brief Returns current (possibly, mocked) timestamp as a @c google::protobuf::Timestamp.
google::protobuf::Timestamp NowTimestamp();

/// @brief Checks if @c google::type::Date contains a valid value according to protobuf documentation.
///
/// Year must be from 1 to 9999.
/// Month must be from 1 to 12.
/// Day must be from 1 to 31 and valid for the year and month.
bool IsValid(const google::type::Date& grpc_date);

/// @brief Exception thrown when date conversion functions receive/produce invalid @c google::type::Date
/// accrording to @c IsValid or otherwise lead to undefined behavior due to integer overflow.
class DateConversionError : public std::overflow_error {
    using std::overflow_error::overflow_error;
};

#if __cpp_lib_chrono >= 201907L

/// @brief Creates @c google::type::Date from @c std::chrono::year_month_day.
///
/// @throws DateConversionError if the value is not valid according to @c IsValid.
google::type::Date ToProtoDate(const std::chrono::year_month_day& system_date);

/// @brief Creates @c std::chrono::year_month_day from @c google::type::Date.
///
/// @throws DateConversionError if the value is not valid according to @c IsValid.
std::chrono::year_month_day ToYearMonthDay(const google::type::Date& grpc_date);

#endif

/// @brief Creates @c google::type::Date from @c utils::datetime::Date.
///
/// @throws DateConversionError if the value is not valid according to @c IsValid.
google::type::Date ToProtoDate(const utils::datetime::Date& utils_date);

/// @brief Creates @c utils::datetime::Date from @c google::type::Date.
///
/// @throws DateConversionError if the value is not valid according to @c IsValid.
utils::datetime::Date ToUtilsDate(const google::type::Date& grpc_date);

/// @brief Creates @c google::type::Date from @c std::chrono::time_point.
///
/// @throws DateConversionError if the value is not valid according to @c IsValid.
template <class Duration>
google::type::Date ToProtoDate(const std::chrono::time_point<std::chrono::system_clock, Duration>& system_tp) {
    return ToProtoDate(utils::datetime::Date(std::chrono::floor<utils::datetime::Date::Days>(system_tp)));
}

/// @brief Creates @c std::chrono::system_clock::time_point from @c google::type::Date.
///
/// @throws DateConversionError if the value is not valid according to @c IsValid.
std::chrono::time_point<std::chrono::system_clock, utils::datetime::Days> ToTimePoint(
    const google::type::Date& grpc_date
);

/// @brief Returns current (possibly, mocked) timestamp as a @c google::type::Date.
google::type::Date NowDate();

/// @brief Checks if @c google::protobuf::Duration contains a valid value according to protobuf documentation.
///
/// Seconds must be from -315,576,000,000 to +315,576,000,000 inclusive.
/// Note: these bounds are computed from: 60 sec/min * 60 min/hr * 24 hr/day * 365.25 days/year * 10000 years
/// For durations of one second or more, a non-zero value for the `nanos` field must be of the same sign as `seconds`.
/// Nanos must also be from -999,999,999 to +999,999,999 inclusive.
bool IsValid(const google::protobuf::Duration& grpc_duration);

/// @brief Exception thrown when duration conversion functions receive/produce invalid @c google::protobuf::Duration
/// accrording to @c IsValid or otherwise lead to undefined behavior due to integer overflow.
class DurationConversionError : public std::overflow_error {
    using std::overflow_error::overflow_error;
};

/// @brief Creates @c std::chrono::duration from @c google::protobuf::Duration.
///
/// @throws DurationConversionError if the value is not valid according to @c IsValid or does not fit the Duration.
template <class Duration = std::chrono::microseconds>
Duration ToDuration(const google::protobuf::Duration& grpc_duration) {
    if (!IsValid(grpc_duration)) {
        throw DurationConversionError("grpc_duration is invalid");
    }

    const std::chrono::seconds seconds(grpc_duration.seconds());
    // Equality is important here. Otherwise, grpc_duration.seconds() == std::chrono::seconds::max()
    // will lead to overflow if grpc_duration.nanos() != 0.
    if (seconds >= std::chrono::duration_cast<std::chrono::seconds>(Duration::max())) {
        throw DurationConversionError("grpc_duration does not fit the output type");
    }
    return std::chrono::duration_cast<Duration>(
        seconds + std::chrono::duration_cast<Duration>(std::chrono::nanoseconds(grpc_duration.nanos()))
    );
}

/// @brief Creates @c google::protobuf::Duration from @c std::chrono::duration.
///
/// @throws DurationConversionError if the value is not valid according to @c IsValid.
template <class Rep, class Period>
google::protobuf::Duration ToProtoDuration(const std::chrono::duration<Rep, Period>& duration) {
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    const auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(duration - seconds);

    google::protobuf::Duration result;
    result.set_seconds(seconds.count());
    result.set_nanos(nanos.count());
    if (!IsValid(result)) {
        throw DurationConversionError("duration is invalid");
    }
    return result;
}

}  // namespace ugrpc

namespace formats::parse {

google::protobuf::Timestamp Parse(const formats::json::Value& json, formats::parse::To<google::protobuf::Timestamp>);

google::type::Date Parse(const formats::json::Value& json, formats::parse::To<google::type::Date>);

}  // namespace formats::parse

namespace formats::serialize {

formats::json::Value Serialize(const google::protobuf::Timestamp& value, formats::serialize::To<formats::json::Value>);

formats::json::Value Serialize(const google::type::Date& value, formats::serialize::To<formats::json::Value>);

}  // namespace formats::serialize

USERVER_NAMESPACE_END
