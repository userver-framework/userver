#include <userver/utils/datetime/date.hpp>

#include <userver/logging/log.hpp>
#include <userver/utils/datetime/cpp_20_calendar.hpp>
#include <userver/utils/datetime/from_string_saturating.hpp>

#include <cctz/time_zone.h>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {

namespace {
const std::string kDateFormat = "%Y-%m-%d";
const auto kUtcTz = cctz::utc_time_zone();

constexpr std::chrono::days ToDays(int year, int month, int day) noexcept {
    const std::chrono::year_month_day ymd{
        std::chrono::year{year},
        std::chrono::month{static_cast<unsigned>(month)},
        std::chrono::day{static_cast<unsigned>(day)}
    };

    return std::chrono::sys_days{ymd}.time_since_epoch();
}

}  // namespace

Date::Date(int year, int month, int day)
    : sys_days_{ToDays(year, month, day)}
{}

Date DateFromRFC3339String(const std::string& date_string) {
    const auto time_point = FromStringSaturating(date_string, kDateFormat);
    return {std::chrono::time_point_cast<Date::Days>(time_point)};
}

std::string ToString(Date date) {
    return cctz::format(
        kDateFormat,
        std::chrono::time_point_cast<std::chrono::system_clock::duration>(date.GetSysDays()),
        kUtcTz
    );
}

std::ostream& operator<<(std::ostream& os, Date date) { return os << ToString(date); }

}  // namespace utils::datetime

USERVER_NAMESPACE_END
