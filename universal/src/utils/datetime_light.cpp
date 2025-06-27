#include <userver/utils/datetime_light.hpp>

#include <array>
#include <ctime>
#include <optional>

#include <sys/param.h>

#include <cctz/time_zone.h>

#include <userver/utils/assert.hpp>
#include <userver/utils/from_string.hpp>
#include <userver/utils/mock_now.hpp>
#include <userver/utils/numeric_cast.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {

namespace {

// https://msdn.microsoft.com/en-us/library/system.datetime.maxvalue(v=vs.110).aspx
constexpr std::int64_t kMaxDotNetTicks = 3155378975999999999L;

// https://msdn.microsoft.com/en-us/library/z2xf7zzk(v=vs.110).aspx
// python:
// (datetime.datetime(1970, 1, 1) - datetime.datetime(1, 1, 1)).total_seconds()
constexpr std::int64_t k100NanosecondsIntervalsBetweenDotNetAndPosixTimeStart =
    62135596800L * 10000000L;  // sec to 100nanosec
constexpr std::int64_t kNanosecondsIs100Nanoseconds = 100;

const cctz::time_zone& GetLocalTimezone() {
    static const cctz::time_zone kLocalTz = cctz::local_time_zone();
    return kLocalTz;
}

}  // namespace

DateParseError::DateParseError(const std::string& timestring)
    : std::runtime_error("Can't parse datetime: " + timestring) {}

TimezoneLookupError::TimezoneLookupError(const std::string& tzname)
    : std::runtime_error("Can't load time zone: " + tzname) {}

bool IsTimeBetween(
    int hour,
    int min,
    int hour_from,
    int min_from,
    int hour_to,
    int min_to,
    bool include_time_to
) noexcept {
    const bool greater_that_time_from = hour > hour_from || (hour == hour_from && min >= min_from);

    const bool lower_than_time_to =
        hour < hour_to || (hour == hour_to && (min < min_to || (include_time_to && min == min_to)));

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

std::string LocalTimezoneTimestring(std::chrono::system_clock::time_point tp, const std::string& format) {
    return cctz::format(format, tp, GetLocalTimezone());
}

std::string UtcTimestring(std::time_t timestamp, const std::string& format) {
    auto tp = std::chrono::system_clock::from_time_t(timestamp);
    return cctz::format(format, tp, cctz::utc_time_zone());
}

std::string Timestring(std::chrono::system_clock::time_point tp) { return UtcTimestring(tp); }

std::string LocalTimezoneTimestring(time_t timestamp, const std::string& format) {
    auto tp = std::chrono::system_clock::from_time_t(timestamp);
    return LocalTimezoneTimestring(tp, format);
}

std::optional<std::chrono::system_clock::time_point>
OptionalStringtime(const std::string& timestring, const cctz::time_zone& timezone, const std::string& format) {
    std::chrono::system_clock::time_point tp;
    if (cctz::parse(format, timestring, timezone, &tp)) {
        return tp;
    }
    return {};
}

std::optional<std::chrono::system_clock::time_point> OptionalStringtime(const std::string& timestring) {
    return OptionalStringtime(timestring, cctz::utc_time_zone(), kDefaultFormat);
}

std::chrono::system_clock::time_point
LocalTimezoneStringtime(const std::string& timestring, const std::string& format) {
    const auto optional_tp = OptionalStringtime(timestring, GetLocalTimezone(), format);
    if (!optional_tp) {
        throw DateParseError(timestring);
    }
    return *optional_tp;
}

std::chrono::system_clock::time_point UtcStringtime(const std::string& timestring, const std::string& format) {
    const auto optional_tp = OptionalStringtime(timestring, cctz::utc_time_zone(), format);
    if (!optional_tp) {
        throw DateParseError(timestring);
    }
    return *optional_tp;
}

std::chrono::system_clock::time_point Stringtime(const std::string& timestring) { return UtcStringtime(timestring); }

std::chrono::system_clock::time_point
DoGuessStringtime(const std::string& timestring, const cctz::time_zone& timezone) {
    static const std::array<std::string, 3> formats{
        {"%Y-%m-%dT%H:%M:%E*S%Ez", "%Y-%m-%dT%H:%M:%E*S%z", "%Y-%m-%dT%H:%M:%E*SZ"}};
    for (const auto& format : formats) {
        const auto optional_tp = OptionalStringtime(timestring, timezone, format);
        if (optional_tp) {
            return *optional_tp;
        }
    }
    throw utils::datetime::DateParseError(timestring);
}

std::chrono::system_clock::time_point GuessLocalTimezoneStringtime(const std::string& timestamp) {
    return DoGuessStringtime(timestamp, GetLocalTimezone());
}

time_t Timestamp(std::chrono::system_clock::time_point tp) noexcept { return std::chrono::system_clock::to_time_t(tp); }

time_t Timestamp() noexcept { return Timestamp(Now()); }

std::uint32_t ParseDayTime(const std::string& str) {
    // Supported day time formats
    // hh:mm:ss
    // hh:mm
    if ((str.size() != 5 && str.size() != 8))
        throw std::invalid_argument(std::string("Failed to parse time from ") + str);

    if (str[2] != ':' || (str.size() == 8 && str[5] != ':'))
        throw std::invalid_argument(std::string("Failed to parse time from ") + str);
    std::uint8_t hours = 0;
    std::uint8_t minutes = 0;
    std::uint8_t seconds = 0;

    try {
        hours = utils::numeric_cast<std::uint8_t>(utils::FromString<int>(std::string_view{str}.substr(0, 2)));
        minutes = utils::numeric_cast<std::uint8_t>(utils::FromString<int>(std::string_view{str}.substr(3, 2)));
        if (str.size() == 8)
            seconds = utils::numeric_cast<std::uint8_t>(utils::FromString<int>(std::string_view{str}.substr(6, 2)));

    } catch (const std::exception& ex) {
        throw std::invalid_argument(std::string("Failed to parse time from ") + str);
    }

    if (hours > 23 || minutes > 59 || seconds > 59)
        throw std::invalid_argument(std::string("Failed to parse time from ") + str);

    return seconds + 60 * minutes + 3600 * hours;
}

cctz::civil_second LocalTimezoneLocalize(const std::chrono::system_clock::time_point& tp) {
    return cctz::convert(tp, GetLocalTimezone());
}

time_t LocalTimezoneUnlocalize(const cctz::civil_second& local_tp) {
    return Timestamp(cctz::convert(local_tp, GetLocalTimezone()));
}

std::chrono::steady_clock::time_point SteadyNow() noexcept { return MockSteadyNow(); }

std::chrono::system_clock::time_point Now() noexcept { return MockNow(); }

WallCoarseClock::time_point WallCoarseNow() noexcept { return MockWallCoarseNow(); }

std::chrono::system_clock::time_point Epoch() noexcept {
    return std::chrono::system_clock::from_time_t(kStartOfTheEpoch);
}

/// Return string with time in ISO8601 format "YYYY-MM-DDTHH:MM:SS+0000".
std::string TimestampToString(const time_t timestamp) {
    static constexpr size_t kStringLen = 24;  // "YYYY-MM-DDTHH:MM:SS+0000"

    std::tm ptm{};
    gmtime_r(&timestamp, &ptm);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init): performance
    std::array<char, kStringLen + 1> buffer;
    auto ret = strftime(buffer.data(), buffer.size(), "%Y-%m-%dT%H:%M:%S+0000", &ptm);
    UASSERT(ret == kStringLen);
    return {buffer.data(), kStringLen};
}

std::int64_t TimePointToTicks(const std::chrono::system_clock::time_point& tp) noexcept {
    if (tp == std::chrono::system_clock::time_point::max()) return kMaxDotNetTicks;
    return k100NanosecondsIntervalsBetweenDotNetAndPosixTimeStart +
           std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count() /
               kNanosecondsIs100Nanoseconds;
}

std::chrono::system_clock::time_point TicksToTimePoint(std::int64_t ticks) noexcept {
    if (ticks == kMaxDotNetTicks) return std::chrono::system_clock::time_point::max();
    return std::chrono::system_clock::time_point(
        std::chrono::duration_cast<std::chrono::system_clock::time_point::duration>(std::chrono::nanoseconds(
            (ticks - k100NanosecondsIntervalsBetweenDotNetAndPosixTimeStart) * kNanosecondsIs100Nanoseconds
        ))
    );
}

}  // namespace utils::datetime

USERVER_NAMESPACE_END
