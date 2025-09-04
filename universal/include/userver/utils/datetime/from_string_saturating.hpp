#pragma once

/// @file userver/utils/datetime/from_string_saturating.hpp
/// @brief Saturating converters from strings to time points.
/// @ingroup userver_universal

#include <chrono>
#include <string>

#include <cctz/time_zone.h>

#include <userver/utils/datetime/cpp_20_calendar.hpp>
#include <userver/utils/datetime_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {

/// @brief Converts strings of the specified format starting with "%Y" to
/// std::chrono::system_clock::time_point in UTC timezone and saturates on overflow.
template <class Duration = std::chrono::system_clock::duration>
std::chrono::time_point<std::chrono::system_clock, Duration>
FromStringSaturating(const std::string& timestring, const std::string& format) {
    using TimePoint = std::chrono::time_point<std::chrono::system_clock, Duration>;

    constexpr cctz::time_point<Days> kTaxiInfinity{DaysBetweenYears(1970, 10000)};

    // reimplement cctz::parse() because we cannot distinguish overflow otherwise
    cctz::time_point<cctz::seconds> tp_seconds;
    cctz::detail::femtoseconds femtoseconds;
    if (!cctz::detail::parse(format, timestring, cctz::utc_time_zone(), &tp_seconds, &femtoseconds)) {
        throw DateParseError(timestring);
    }

    // manually cast to a coarser time_point
    if (std::chrono::time_point_cast<Days>(tp_seconds) >= kTaxiInfinity ||
        tp_seconds > std::chrono::time_point_cast<decltype(tp_seconds)::duration>(TimePoint::max())) {
        return TimePoint::max();
    }

    return std::chrono::time_point_cast<Duration>(tp_seconds) + std::chrono::duration_cast<Duration>(femtoseconds);
}

/// @brief Converts strings like "2012-12-12T00:00:00" to
/// std::chrono::system_clock::time_point in UTC timezone and saturates on overflow
/// Example:
/// @snippet utils/datetime/from_string_saturating_test.cpp FromStringSaturation
template <class Duration = std::chrono::system_clock::duration>
std::chrono::time_point<std::chrono::system_clock, Duration> FromRfc3339StringSaturating(const std::string& timestring
) {
    return FromStringSaturating<Duration>(timestring, kRfc3339Format);
}

}  // namespace utils::datetime

USERVER_NAMESPACE_END
