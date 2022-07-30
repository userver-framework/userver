#pragma once

/// @file userver/utils/datetime.hpp
/// @brief Date and Time related converters

#include <chrono>
#include <stdexcept>
#include <string>

#include <cctz/civil_time.h>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {
/// @ingroup userver_constants
/// @{
inline const std::string kRfc3339Format = "%Y-%m-%dT%H:%M:%E*S%Ez";
inline const std::string kTaximeterFormat = "%Y-%m-%dT%H:%M:%E6SZ";
inline constexpr std::time_t kStartOfTheEpoch = 0;
inline const std::string kDefaultDriverTimezone = "Europe/Moscow";
inline const std::string kDefaultTimezone = "UTC";
inline const std::string kDefaultFormat = "%Y-%m-%dT%H:%M:%E*S%z";
inline const std::string kIsoFormat = "%Y-%m-%dT%H:%M:%SZ";
/// @}

using timepair_t = std::pair<uint8_t, uint8_t>;

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
std::chrono::system_clock::time_point Now() noexcept;

/// @brief Returns std::chrono::system_clock::time_point from the start of the
/// epoch
std::chrono::system_clock::time_point Epoch() noexcept;

/// @brief std::chrono::steady_clock::now() that could be mocked
///
/// It is only intended for period-based structures/algorithms testing.
///
/// @warning You MUST NOT pass time points received from this function outside
/// of your own code. Otherwise this will break your service in production.
std::chrono::steady_clock::time_point SteadyNow() noexcept;

// See the comment to SteadyNow()
class SteadyClock : public std::chrono::steady_clock {
 public:
  using time_point = std::chrono::steady_clock::time_point;

  static time_point now() { return SteadyNow(); }
};

/// @brief Returns true if the time is in range; works over midnight too
bool IsTimeBetween(int hour, int min, int hour_from, int min_from, int hour_to,
                   int min_to, bool include_time_to = false) noexcept;

/// @brief Returns time in a string of specified format
/// @throws utils::datetime::TimezoneLookupError
std::string Timestring(std::time_t timestamp,
                       const std::string& timezone = kDefaultTimezone,
                       const std::string& format = kDefaultFormat);

/// @brief Returns time in a string of specified format
std::string LocalTimezoneTimestring(std::time_t timestamp,
                                    const std::string& format = kDefaultFormat);

/// @brief Returns time in a string of specified format
/// @throws utils::datetime::TimezoneLookupError
std::string Timestring(std::chrono::system_clock::time_point tp,
                       const std::string& timezone = kDefaultTimezone,
                       const std::string& format = kDefaultFormat);

/// @brief Returns time in a string of specified format
std::string LocalTimezoneTimestring(std::chrono::system_clock::time_point tp,
                                    const std::string& format = kDefaultFormat);

/// @brief Extracts time point from a string of a specified format
/// @throws utils::datetime::DateParseError
/// @throws utils::datetime::TimezoneLookupError
std::chrono::system_clock::time_point Stringtime(
    const std::string& timestring,
    const std::string& timezone = kDefaultTimezone,
    const std::string& format = kDefaultFormat);

/// @brief Extracts time point from a string of a specified format
/// @throws utils::datetime::DateParseError
std::chrono::system_clock::time_point LocalTimezoneStringtime(
    const std::string& timestring, const std::string& format = kDefaultFormat);

/// @brief Extracts time point from a string, guessing the format
/// @throws utils::datetime::DateParseError
/// @throws utils::datetime::TimezoneLookupError
std::chrono::system_clock::time_point GuessStringtime(
    const std::string& timestamp, const std::string& timezone);

/// @brief Extracts time point from a string, guessing the format
/// @throws utils::datetime::DateParseError
std::chrono::system_clock::time_point GuessLocalTimezoneStringtime(
    const std::string& timestamp);

/// @brief Converts time point to std::time_t
std::time_t Timestamp(std::chrono::system_clock::time_point tp) noexcept;

/// @brief Returned current time as std::time_t; could be mocked
std::time_t Timestamp() noexcept;

/// @brief Parse day time in hh:mm[:ss] format
/// @param str day time in format hh:mm[:ss]
/// @return number of second since start of day
std::uint32_t ParseDayTime(const std::string& str);

/// @brief Converts absolute time in std::chrono::system_clock::time_point to
/// a civil time of a particular timezone.
/// @throws utils::datetime::TimezoneLookupError
cctz::civil_second Localize(const std::chrono::system_clock::time_point& tp,
                            const std::string& timezone);

/// @brief Converts absolute time in std::chrono::system_clock::time_point to
/// a civil time of a local timezone.
cctz::civil_second LocalTimezoneLocalize(
    const std::chrono::system_clock::time_point& tp);

/// @brief Converts a civil time in a specified timezone into an absolute time.
/// @throws utils::datetime::TimezoneLookupError
std::time_t Unlocalize(const cctz::civil_second& local_tp,
                       const std::string& timezone);

/// @brief Converts a civil time in a local timezone into an absolute time.
std::time_t LocalTimezoneUnlocalize(const cctz::civil_second& local_tp);

/// @brief Returns string with time in ISO8601 format "YYYY-MM-DDTHH:MM:SS+0000"
/// @param timestamp unix timestamp
std::string TimestampToString(std::time_t timestamp);

/// @brief Convert time_point to DotNet ticks
/// @param time point day time
/// @return number of 100nanosec intervals between current date and 01/01/0001
int64_t TimePointToTicks(
    const std::chrono::system_clock::time_point& tp) noexcept;

/// @brief Convert DotNet ticks to a time point
std::chrono::system_clock::time_point TicksToTimePoint(int64_t ticks) noexcept;

/// @brief Compute (a - b) with a specified duration
template <class Duration, class Clock>
double CalcTimeDiff(const std::chrono::time_point<Clock>& a,
                    const std::chrono::time_point<Clock>& b) {
  const auto duration_a = a.time_since_epoch();
  const auto duration_b = b.time_since_epoch();
  return std::chrono::duration_cast<Duration>(duration_a - duration_b).count();
}

}  // namespace utils::datetime

USERVER_NAMESPACE_END
