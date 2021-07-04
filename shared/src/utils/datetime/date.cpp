#include <userver/utils/datetime/date.hpp>

#include <userver/logging/log.hpp>
#include <userver/utils/datetime/from_string_saturating.hpp>

#include <cctz/time_zone.h>

namespace utils::datetime {

namespace {
const std::string kDateFormat = "%Y-%m-%d";
const auto kUtcTz = cctz::utc_time_zone();

// TODO: replace with C++20 std::chrono::days
using Days = std::chrono::duration<
    int, std::ratio_multiply<std::ratio<24>, std::chrono::hours::period>>;

// https://howardhinnant.github.io/date_algorithms.html#days_from_civil
constexpr Days to_days(int year, int month, int day) noexcept {
  static_assert(std::numeric_limits<unsigned>::digits >= 18);
  static_assert(std::numeric_limits<int>::digits >= 20);

  const auto yr = year - (month <= 2 /*February*/);
  const auto mth = static_cast<unsigned>(month);
  const auto dy = static_cast<unsigned>(day);

  const auto era = (yr >= 0 ? yr : yr - 399) / 400;
  const auto yoe = static_cast<unsigned>(yr - era * 400);  // [0, 399]
  const auto doy =
      (153 * (mth + (mth > 2 ? -3 : 9)) + 2) / 5 + dy - 1;  // [0, 365]
  const auto doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;   // [0, 146096]
  return Days{era * 146097 + static_cast<int>(doe) - 719468};
}

}  // namespace

Date::Date(int year, int month, int day)
    : sys_days_{to_days(year, month, day)} {}

Date DateFromRFC3339String(const std::string& date_string) {
  const auto time_point = FromStringSaturating(date_string, kDateFormat);
  return Date(std::chrono::time_point_cast<Date::Days>(time_point));
}

std::string ToString(Date date) {
  return cctz::format(
      kDateFormat,
      std::chrono::time_point_cast<std::chrono::system_clock::duration>(
          date.GetSysDays()),
      kUtcTz);
}

std::ostream& operator<<(std::ostream& os, Date date) {
  return os << ToString(date);
}

}  // namespace utils::datetime
