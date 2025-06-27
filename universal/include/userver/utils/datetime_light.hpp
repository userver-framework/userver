#pragma once

/// @file userver/utils/datetime_light.hpp
/// @brief Date and Time related converters
/// @ingroup userver_universal

#include <chrono>
#include <cstdint>
#include <optional>
#include <ratio>
#include <stdexcept>
#include <string>

#include <cctz/civil_time.h>
#include <cctz/time_zone.h>

#include <userver/utils/datetime/wall_coarse_clock.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {
/// @snippet utils/datetime/from_string_saturating_test.cpp  kRfc3339Format
inline const std::string kRfc3339Format = "%Y-%m-%dT%H:%M:%E*S%Ez";
/// @snippet utils/datetime/from_string_saturating_test.cpp  kTaximeterFormat
inline const std::string kTaximeterFormat = "%Y-%m-%dT%H:%M:%E6SZ";
inline constexpr std::time_t kStartOfTheEpoch = 0;
/// @snippet utils/datetime_test.cpp  kDefaultDriverTimezone
inline const std::string kDefaultDriverTimezone = "Europe/Moscow";
/// @snippet utils/datetime_test.cpp  kDefaultTimezone
inline const std::string kDefaultTimezone = "UTC";
/// @snippet utils/datetime/from_string_saturating_test.cpp  kDefaultFormat
inline const std::string kDefaultFormat = "%Y-%m-%dT%H:%M:%E*S%z";
/// @snippet utils/datetime/from_string_saturating_test.cpp  kIsoFormat
inline const std::string kIsoFormat = "%Y-%m-%dT%H:%M:%SZ";
inline const std::string kFractionFormat = "%Y-%m-%dT%H:%M:%S.%E*f%z";

using timepair_t = std::pair<std::uint8_t, std::uint8_t>;

/// Date/time parsing error
class DateParseError : public std::runtime_error {
public:
    DateParseError(const std::string& timestring);
};

/// Timezone information lookup error
class TimezoneLookupError : public std::runtime_error {
public:
    TimezoneLookupError(const std::string& tzname);
};

/// @brief std::chrono::system_clock::now() that could be mocked
///
/// Returns last time point passed to utils::datetime::MockNowSet(), or
/// std::chrono::system_clock::now() if the timepoint is not mocked.
std::chrono::system_clock::time_point Now() noexcept;

/// @brief Returns std::chrono::system_clock::time_point from the start of the
/// epoch
std::chrono::system_clock::time_point Epoch() noexcept;

/// @brief std::chrono::steady_clock::now() that could be mocked
///
/// Returns last time point passed to utils::datetime::MockNowSet(), or
/// std::chrono::steady_clock::now() if the timepoint is not mocked.
///
/// It is only intended for period-based structures/algorithms testing.
///
/// @warning You MUST NOT pass time points received from this function outside
/// of your own code. Otherwise this will break your service in production.
std::chrono::steady_clock::time_point SteadyNow() noexcept;

/// @brief utils::datetime::WallCoarseClock::now() that could be mocked
///
/// Returns last time point passed to utils::datetime::MockNowSet(), or utils::datetime::WallCoarseClock::now() if
/// the timepoint is not mocked.
WallCoarseClock::time_point WallCoarseNow() noexcept;

// See the comment to SteadyNow()
class SteadyClock : public std::chrono::steady_clock {
public:
    using time_point = std::chrono::steady_clock::time_point;

    static time_point now() { return SteadyNow(); }
};

/// @brief Returns true if the time is in range; works over midnight too
bool IsTimeBetween(
    int hour,
    int min,
    int hour_from,
    int min_from,
    int hour_to,
    int min_to,
    bool include_time_to = false
) noexcept;

/// @brief Extracts time point from a string, guessing the format
/// @note Use GuessStringtime instead
/// @throws utils::datetime::DateParseError
std::chrono::system_clock::time_point DoGuessStringtime(const std::string& timestring, const cctz::time_zone& timezone);

/// @brief Returns time in a string of specified format in local timezone
/// @see kRfc3339Format, kTaximeterFormat, kStartOfTheEpoch,
/// kDefaultDriverTimezone, kDefaultTimezone, kDefaultFormat, kIsoFormat
std::string LocalTimezoneTimestring(std::time_t timestamp, const std::string& format = kDefaultFormat);

/// @brief Returns time in a string of specified format in UTC timezone
/// @see kRfc3339Format, kTaximeterFormat, kStartOfTheEpoch,
/// kDefaultDriverTimezone, kDefaultTimezone, kDefaultFormat, kIsoFormat
std::string UtcTimestring(std::time_t timestamp, const std::string& format = kDefaultFormat);

std::string Timestring(std::chrono::system_clock::time_point tp);

/// @brief Returns time in a string of specified format in local timezone
/// @see kRfc3339Format, kTaximeterFormat, kStartOfTheEpoch,
/// kDefaultDriverTimezone, kDefaultTimezone, kDefaultFormat, kIsoFormat
std::string
LocalTimezoneTimestring(std::chrono::system_clock::time_point tp, const std::string& format = kDefaultFormat);

/// @brief Returns time in a string of specified format in UTC timezone
/// @see kRfc3339Format, kTaximeterFormat, kStartOfTheEpoch,
/// kDefaultDriverTimezone, kDefaultTimezone, kDefaultFormat, kIsoFormat
/// @throws std::runtime_error if tp does not fit into @c std::chrono::hours
template <class Duration>
std::string UtcTimestring(
    std::chrono::time_point<std::chrono::system_clock, Duration> tp,
    const std::string& format = kDefaultFormat
) {
    using quotient = std::ratio_divide<typename Duration::period, std::chrono::hours::period>;
    if constexpr (quotient::num <= quotient::den) {
        return cctz::format(format, tp, cctz::utc_time_zone());
    } else {
        // cctz cannot handle time_points with duration more than hours
        const auto days = tp.time_since_epoch();
        if (days > std::chrono::duration_cast<Duration>(std::chrono::hours::max())) {
            throw std::runtime_error("tp does not find std::chrono::hours");
        }
        const std::chrono::time_point<std::chrono::system_clock, std::chrono::hours> tp_hours(
            std::chrono::duration_cast<std::chrono::hours>(days)
        );
        return cctz::format(format, tp_hours, cctz::utc_time_zone());
    }
}

/// @brief Extracts time point from a string of a specified format in local timezone
/// @throws utils::datetime::DateParseError
/// @see kRfc3339Format, kTaximeterFormat, kStartOfTheEpoch,
/// kDefaultDriverTimezone, kDefaultTimezone, kDefaultFormat, kIsoFormat
std::chrono::system_clock::time_point
LocalTimezoneStringtime(const std::string& timestring, const std::string& format = kDefaultFormat);

/// @brief Extracts time point from a string of a specified format in UTC timezone
/// @throws utils::datetime::DateParseError
/// @see kRfc3339Format, kTaximeterFormat, kStartOfTheEpoch,
/// kDefaultDriverTimezone, kDefaultTimezone, kDefaultFormat, kIsoFormat
std::chrono::system_clock::time_point
UtcStringtime(const std::string& timestring, const std::string& format = kDefaultFormat);

/// @brief Extracts time point from a string of a kDefaultFormat format in UTC timezone
/// @throws utils::datetime::DateParseError
std::chrono::system_clock::time_point Stringtime(const std::string& timestring);

/// @brief Extracts time point from a string of a specified format
/// @throws utils::datetime::DateParseError
/// @see kRfc3339Format, kTaximeterFormat, kStartOfTheEpoch,
/// kDefaultDriverTimezone, kDefaultTimezone, kDefaultFormat, kIsoFormat
std::optional<std::chrono::system_clock::time_point>
OptionalStringtime(const std::string& timestring, const cctz::time_zone& timezone, const std::string& format);

/// @brief Extracts time point from a string of a kDefaultFormat format in UTC timezone
/// @throws utils::datetime::DateParseError
std::optional<std::chrono::system_clock::time_point> OptionalStringtime(const std::string& timestring);

/// @brief Extracts time point from a string, guessing the format
/// @throws utils::datetime::DateParseError
std::chrono::system_clock::time_point GuessLocalTimezoneStringtime(const std::string& timestamp);

/// @brief Converts time point to std::time_t
///
/// Example:
///
/// @snippet utils/datetime_test.cpp  UtcTimestring C time example
std::time_t Timestamp(std::chrono::system_clock::time_point tp) noexcept;

/// @brief Returned current time as std::time_t; could be mocked
std::time_t Timestamp() noexcept;

/// @brief Parse day time in hh:mm[:ss] format
/// @param str day time in format hh:mm[:ss]
/// @return number of second since start of day
std::uint32_t ParseDayTime(const std::string& str);

/// @brief Converts absolute time in std::chrono::system_clock::time_point to
/// a civil time of a local timezone.
cctz::civil_second LocalTimezoneLocalize(const std::chrono::system_clock::time_point& tp);

/// @brief Converts a civil time in a local timezone into an absolute time.
std::time_t LocalTimezoneUnlocalize(const cctz::civil_second& local_tp);

/// @brief Returns string with time in ISO8601 format "YYYY-MM-DDTHH:MM:SS+0000"
/// @param timestamp unix timestamp
std::string TimestampToString(std::time_t timestamp);

/// @brief Convert time_point to DotNet ticks
/// @param tp time point
/// @return number of 100-nanosecond intervals between current date and 01/01/0001
///
/// Example:
///
/// @snippet utils/datetime_test.cpp  TimePointToTicks example
std::int64_t TimePointToTicks(const std::chrono::system_clock::time_point& tp) noexcept;

/// @brief Convert DotNet ticks to a time point
std::chrono::system_clock::time_point TicksToTimePoint(std::int64_t ticks) noexcept;

/// @brief Compute (a - b) with a specified duration
template <class Duration, class Clock>
double CalcTimeDiff(const std::chrono::time_point<Clock>& a, const std::chrono::time_point<Clock>& b) {
    const auto duration_a = a.time_since_epoch();
    const auto duration_b = b.time_since_epoch();
    return std::chrono::duration_cast<Duration>(duration_a - duration_b).count();
}

}  // namespace utils::datetime

USERVER_NAMESPACE_END
