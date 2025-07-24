#pragma once

/// @file userver/utils/datetime/cpp_20_calendar.hpp
/// @brief This file brings date.h into utils::datetime::date namespace,
/// which is std::chrono::operator/ (calendar) in c++20.
/// @ingroup userver_universal

// TODO : replace with C++20 std::chrono:: when time comes

#include <chrono>
#include <ratio>

#include <date/date.h>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {

namespace date = ::date;

using Days = date::days;
using DaysTimepoint = date::sys_days;

/// @brief Calculates the number of days between January 1, 00:00 of two years accounting for leap years.
constexpr Days DaysBetweenYears(int from, int to) {
    return std::chrono::duration_cast<Days>(
        DaysTimepoint{date::year_month_day(date::year(to), date::month(1), date::day(1))} -
        DaysTimepoint{date::year_month_day(date::year(from), date::month(1), date::day(1))}
    );
}

/// @brief Get the number of days in the given month of a given year.
constexpr date::day DaysInMonth(int month, int year) {
    UINVARIANT(month >= 1 && month <= 12, "Month must be between 1 and 12");
    return date::year_month_day_last(date::year(year), date::month_day_last(date::month(month))).day();
}

}  // namespace utils::datetime

USERVER_NAMESPACE_END
