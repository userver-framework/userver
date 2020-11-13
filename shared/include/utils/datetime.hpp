#pragma once

#include <chrono>
#include <stdexcept>
#include <string>

#include <cctz/civil_time.h>

namespace utils::datetime {

static const std::string kRfc3339Format = "%Y-%m-%dT%H:%M:%E*S%Ez";
static const std::string kTaximeterFormat = "%Y-%m-%dT%H:%M:%E6SZ";
static const std::time_t kStartOfTheEpoch = 0;
static const std::string kDefaultDriverTimezone = "Europe/Moscow";
static const std::string kDefaultTimezone = "UTC";
static const std::string kDefaultFormat = "%Y-%m-%dT%H:%M:%E*S%z";
static const std::string kIsoFormat = "%Y-%m-%dT%H:%M:%SZ";

using timepair_t = std::pair<uint8_t, uint8_t>;

class DateParseError : public std::runtime_error {
 public:
  DateParseError(const std::string& timestring);
};

std::chrono::system_clock::time_point Now();
std::chrono::system_clock::time_point Epoch();

// You MUST NOT pass time points received from this function outside
// of your own code. Otherwise this will break your service in production.
//
// It is only intended for period-based structures/algorithms testing.
std::chrono::steady_clock::time_point SteadyNow();

// See the comment to SteadyNow()
class SteadyClock : public std::chrono::steady_clock {
 public:
  using time_point = std::chrono::steady_clock::time_point;

  static time_point now() { return SteadyNow(); }
};

bool IsTimeBetween(int hour, int min, int hour_from, int min_from, int hour_to,
                   int min_to, bool include_time_to = false);

std::string Timestring(std::time_t timestamp,
                       const std::string& timezone = kDefaultTimezone,
                       const std::string& format = kDefaultFormat);
std::string Timestring(std::chrono::system_clock::time_point tp,
                       const std::string& timezone = kDefaultTimezone,
                       const std::string& format = kDefaultFormat);

std::chrono::system_clock::time_point Stringtime(
    const std::string& timestring,
    const std::string& timezone = kDefaultTimezone,
    const std::string& format = kDefaultFormat);

std::chrono::system_clock::time_point GuessStringtime(
    const std::string& timestamp, const std::string& timezone);

std::time_t Timestamp(std::chrono::system_clock::time_point tp);
std::time_t Timestamp();

/**
 * Parse day time
 * @param str day time in format hh:mm[:ss]
 * @return number of second since start of day
 */
std::uint32_t ParseDayTime(const std::string& str);

cctz::civil_second Localize(const std::chrono::system_clock::time_point& tp,
                            const std::string& timezone);

std::time_t Unlocalize(const cctz::civil_second& local_tp,
                       const std::string& timezone);

/**
 * @param timestamp unix timestamp
 * @return string with time in ISO8601 format "YYYY-MM-DDTHH:MM:SS+0000"
 */
std::string TimestampToString(std::time_t timestamp);

/**
 * Convert time_point to DotNet ticks
 * @param time point day time
 * @return number of 100nanosec intervals between current date and 01/01/0001
 */
int64_t TimePointToTicks(const std::chrono::system_clock::time_point& tp);

std::chrono::system_clock::time_point TicksToTimePoint(int64_t ticks);

template <class T, class Clock>
double CalcTimeDiff(const std::chrono::time_point<Clock>& a,
                    const std::chrono::time_point<Clock>& b) {
  const auto duration_a = a.time_since_epoch();
  const auto duration_b = b.time_since_epoch();
  return std::chrono::duration_cast<T>(duration_a - duration_b).count();
}

}  // namespace utils::datetime
