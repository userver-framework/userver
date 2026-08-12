#pragma once

/// @file userver/utils/datetime/cpp_20_calendar.hpp
/// @brief Helpers for C++20 calendar.
/// @ingroup userver_universal

#include <chrono>
#include <ratio>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

/// @brief Date, time-of-day, clocks, and parsing utilities.
namespace utils::datetime {

// TODO: remove the following aliases
using Days = std::chrono::days;
using DaysTimepoint = std::chrono::sys_days;

/// @brief Calculates the number of days between January 1, 00:00 of two years accounting for leap years.
constexpr std::chrono::days DaysBetweenYears(int from, int to) {
    return std::chrono::duration_cast<Days>(
        DaysTimepoint{std::chrono::year_month_day(std::chrono::year(to), std::chrono::month(1), std::chrono::day(1))} -
        DaysTimepoint{std::chrono::year_month_day(std::chrono::year(from), std::chrono::month(1), std::chrono::day(1))}
    );
}

/// @brief Get the number of days in the given month of a given year.
constexpr std::chrono::day DaysInMonth(int month, int year) {
    UINVARIANT(month >= 1 && month <= 12, "Month must be between 1 and 12");
    return std::chrono::year_month_day_last(
               std::chrono::year(year),
               std::chrono::month_day_last(std::chrono::month(month))
    )
        .day();
}

}  // namespace utils::datetime

USERVER_NAMESPACE_END
