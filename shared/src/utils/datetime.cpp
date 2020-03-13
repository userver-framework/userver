#include <utils/datetime.hpp>

#include <array>
#include <ctime>

#include <cctz/time_zone.h>
#include <boost/lexical_cast.hpp>

#include <utils/mock_now.hpp>

namespace utils {
namespace datetime {

namespace {

// https://msdn.microsoft.com/en-us/library/system.datetime.maxvalue(v=vs.110).aspx
constexpr int64_t kMaxDotNetTicks = 3155378975999999999L;

// https://msdn.microsoft.com/en-us/library/z2xf7zzk(v=vs.110).aspx
// python:
// (datetime.datetime(1970, 1, 1) - datetime.datetime(1, 1, 1)).total_seconds()
constexpr int64_t k100NanosecondsIntervalsBetweenDotNetAndPosixTimeStart =
    62135596800L * 10000000L;  // sec to 100nanosec
constexpr int64_t kNanosecondsIs100Nanoseconds = 100;

}  // namespace

DateParseError::DateParseError(const std::string& timestring)
    : std::runtime_error("Can't parse datetime: " + timestring) {}

bool IsTimeBetween(int hour, int min, int hour_from, int min_from, int hour_to,
                   int min_to, bool include_time_to) {
  const bool greater_that_time_from =
      hour > hour_from || (hour == hour_from && min >= min_from);

  const bool lower_than_time_to =
      hour < hour_to ||
      (hour == hour_to && (min < min_to || (include_time_to && min == min_to)));

  if (greater_that_time_from && lower_than_time_to) {
    return true;
  }

  // over midnight
  if (hour_from > hour_to || (hour_from == hour_to && min_from > min_to)) {
    return IsTimeBetween(hour, min, hour_from, min_from, 23, 59, true) ||
           IsTimeBetween(hour, min, 0, 0, hour_to, min_to, include_time_to);
  }

  if (hour == hour_from && min == min_from) {
    return true;
  }

  return false;
}

std::string Timestring(std::chrono::system_clock::time_point tp,
                       const std::string& timezone, const std::string& format) {
  cctz::time_zone tz;
  load_time_zone(timezone, &tz);
  return cctz::format(format, tp, tz);
}

std::chrono::system_clock::time_point Stringtime(const std::string& timestring,
                                                 const std::string& timezone,
                                                 const std::string& format) {
  std::chrono::system_clock::time_point tp;
  cctz::time_zone tz;
  if (!load_time_zone(timezone, &tz)) {
    throw std::runtime_error("Can't load time zone: " + timezone);
  }
  if (!cctz::parse(format, timestring, tz, &tp)) {
    throw DateParseError(timestring);
  }
  return tp;
}

std::string Timestring(time_t timestamp, const std::string& timezone,
                       const std::string& format) {
  auto tp = std::chrono::system_clock::from_time_t(timestamp);
  return Timestring(tp, timezone, format);
}

time_t Timestamp(std::chrono::system_clock::time_point tp) {
  return std::chrono::system_clock::to_time_t(tp);
}

time_t Timestamp() { return Timestamp(Now()); }

std::chrono::system_clock::time_point GuessStringtime(
    const std::string& timestamp, const std::string& timezone) {
  static const std::array<std::string, 3> formats{{"%Y-%m-%dT%H:%M:%E*S%Ez",
                                                   "%Y-%m-%dT%H:%M:%E*S%z",
                                                   "%Y-%m-%dT%H:%M:%E*SZ"}};
  for (const auto& format : formats) {
    try {
      return utils::datetime::Stringtime(timestamp, timezone, format);
    } catch (const utils::datetime::DateParseError&) {
    }
  }

  throw utils::datetime::DateParseError(timestamp);
}

std::uint32_t ParseDayTime(const std::string& str) {
  // Supported day time formats
  // hh:mm:ss
  // hh:mm
  if ((str.size() != 5 && str.size() != 8))
    throw std::invalid_argument(std::string("Failed to parse time from ") +
                                str);

  if (str[2] != ':' || (str.size() == 8 && str[5] != ':'))
    throw std::invalid_argument(std::string("Failed to parse time from ") +
                                str);
  uint8_t hours = 0;
  uint8_t minutes = 0;
  uint8_t seconds = 0;

  try {
    hours = boost::numeric_cast<uint8_t>(
        boost::lexical_cast<int>(std::string(str, 0, 2)));
    minutes = boost::numeric_cast<uint8_t>(
        boost::lexical_cast<int>(std::string(str, 3, 2)));
    if (str.size() == 8)
      seconds = boost::numeric_cast<uint8_t>(
          boost::lexical_cast<int>(std::string(str, 6, 2)));

  } catch (const std::exception& ex) {
    throw std::invalid_argument(std::string("Failed to parse time from ") +
                                str);
  }

  if (hours > 23 || minutes > 59 || seconds > 59)
    throw std::invalid_argument(std::string("Failed to parse time from ") +
                                str);

  return seconds + 60 * minutes + 3600 * hours;
}

cctz::civil_second Localize(const std::chrono::system_clock::time_point& tp,
                            const std::string& timezone) {
  cctz::time_zone tz;
  load_time_zone(timezone, &tz);
  return cctz::convert(tp, tz);
}

time_t Unlocalize(const cctz::civil_second& local_tp,
                  const std::string& timezone) {
  cctz::time_zone tz;
  load_time_zone(timezone, &tz);
  return Timestamp(cctz::convert(local_tp, tz));
}

#ifndef MOCK_NOW
std::chrono::steady_clock::time_point SteadyNow() {
  return std::chrono::steady_clock::now();
}

std::chrono::system_clock::time_point Now() {
  return std::chrono::system_clock::now();
}
#else
std::chrono::steady_clock::time_point SteadyNow() { return MockSteadyNow(); }

std::chrono::system_clock::time_point Now() { return MockNow(); }
#endif

std::chrono::system_clock::time_point Epoch() {
  return std::chrono::system_clock::from_time_t(kStartOfTheEpoch);
}

/// Return string with time in ISO8601 format "YYYY-MM-DDTHH:MM:SS+0000".
std::string TimestampToString(const time_t timestamp) {
  std::tm ptm{};
  gmtime_r(&timestamp, &ptm);
  char buffer[25];  // "YYYY-MM-DDTHH:MM:SS+0000".size() == 24
  strftime(buffer, 25, "%Y-%m-%dT%H:%M:%S+0000", &ptm);
  return std::string(buffer, 24);
}

int64_t TimePointToTicks(const std::chrono::system_clock::time_point& tp) {
  if (tp == std::chrono::system_clock::time_point::max())
    return kMaxDotNetTicks;
  return k100NanosecondsIntervalsBetweenDotNetAndPosixTimeStart +
         std::chrono::duration_cast<std::chrono::nanoseconds>(
             tp.time_since_epoch())
                 .count() /
             kNanosecondsIs100Nanoseconds;
}

std::chrono::system_clock::time_point TicksToTimePoint(int64_t ticks) {
  if (ticks == kMaxDotNetTicks)
    return std::chrono::system_clock::time_point::max();
  return std::chrono::system_clock::time_point(
      std::chrono::duration_cast<
          std::chrono::system_clock::time_point::duration>(
          std::chrono::nanoseconds(
              (ticks - k100NanosecondsIntervalsBetweenDotNetAndPosixTimeStart) *
              kNanosecondsIs100Nanoseconds)));
}

}  // namespace datetime
}  // namespace utils
