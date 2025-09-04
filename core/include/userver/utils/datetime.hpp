#pragma once

/// @file userver/utils/datetime.hpp
/// @brief Date, Time, and Timezone related converters
/// @ingroup userver_universal

#include <userver/utils/datetime_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {

/// @brief Returns time in a string of specified format, for UTC times prefer a faster utils::datetime::UtcTimestring
///
/// @throws utils::datetime::TimezoneLookupError
///
/// Example:
///
/// @snippet utils/datetime_test.cpp  Timestring example
///
/// @see kRfc3339Format, kTaximeterFormat, kStartOfTheEpoch,
/// kDefaultDriverTimezone, kDefaultTimezone, kDefaultFormat, kIsoFormat
std::string Timestring(
    std::time_t timestamp,
    const std::string& timezone = kDefaultTimezone,
    const std::string& format = kDefaultFormat
);

/// @brief Returns time in a string of specified format, for UTC times prefer a faster utils::datetime::UtcTimestring
/// @throws utils::datetime::TimezoneLookupError
///
/// Example:
///
/// @snippet utils/datetime_test.cpp Timestring example
/// @see kRfc3339Format, kTaximeterFormat, kStartOfTheEpoch,
/// kDefaultDriverTimezone, kDefaultTimezone, kDefaultFormat, kIsoFormat
std::string Timestring(
    std::chrono::system_clock::time_point tp,
    const std::string& timezone,
    const std::string& format = kDefaultFormat
);

/// @brief Extracts time point from a string of a specified format, for UTC times prefer a faster
/// utils::datetime::UtcStringtime
///
/// @throws utils::datetime::DateParseError
/// @throws utils::datetime::TimezoneLookupError
///
/// Example:
///
/// @snippet utils/datetime_test.cpp  Stringtime example
/// @see kRfc3339Format, kTaximeterFormat, kStartOfTheEpoch,
/// kDefaultDriverTimezone, kDefaultTimezone, kDefaultFormat, kIsoFormat
std::chrono::system_clock::time_point
Stringtime(const std::string& timestring, const std::string& timezone, const std::string& format = kDefaultFormat);

/// @brief Extracts time point from a string, guessing the format
/// @throws utils::datetime::DateParseError
/// @throws utils::datetime::TimezoneLookupError
///
/// Example:
///
/// @snippet utils/datetime_test.cpp  GuessStringtime example
std::chrono::system_clock::time_point GuessStringtime(const std::string& timestamp, const std::string& timezone);

/// @brief Returns optional time in a string of specified format
///
/// Example:
///
/// @snippet utils/datetime_test.cpp OptionalStringtime example
/// @see kRfc3339Format, kTaximeterFormat, kStartOfTheEpoch,
/// kDefaultDriverTimezone, kDefaultTimezone, kDefaultFormat, kIsoFormat
std::optional<std::chrono::system_clock::time_point> OptionalStringtime(
    const std::string& timestring,
    const std::string& timezone,
    const std::string& format = kDefaultFormat
);

/// @brief Converts absolute time in std::chrono::system_clock::time_point to
/// a civil time of a particular timezone.
/// @throws utils::datetime::TimezoneLookupError
///
/// Example:
///
/// @snippet utils/datetime_test.cpp  Localize example
cctz::civil_second Localize(const std::chrono::system_clock::time_point& tp, const std::string& timezone);

/// @brief Converts a civil time in a specified timezone into an absolute time.
/// @throws utils::datetime::TimezoneLookupError
///
/// Example:
///
/// @snippet utils/datetime_test.cpp  Localize example
std::time_t Unlocalize(const cctz::civil_second& local_tp, const std::string& timezone);

}  // namespace utils::datetime

USERVER_NAMESPACE_END
