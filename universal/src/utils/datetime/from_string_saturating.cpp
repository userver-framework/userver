#include <userver/utils/datetime/from_string_saturating.hpp>

#include <cctz/time_zone.h>

#include <userver/utils/datetime.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {
namespace {

// TODO: in C++20 replace with std::chrono::days
using Days = std::chrono::duration<std::int64_t, std::ratio<24 * 60 * 60>>;

constexpr auto DaysBetweenYears(int64_t from, int64_t to) {
  int64_t days = (to - from) * 365;
  for (auto year = from; year < to; ++year) {
    if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) ++days;
  }
  return Days{days};
}

}  // namespace

std::chrono::system_clock::time_point FromRfc3339StringSaturating(
    const std::string& timestring) {
  return FromStringSaturating(timestring, kRfc3339Format);
}

std::chrono::system_clock::time_point FromStringSaturating(
    const std::string& timestring, const std::string& format) {
  using SystemClock = std::chrono::system_clock;

  constexpr cctz::time_point<Days> kTaxiInfinity{DaysBetweenYears(1970, 10000)};

  // reimplement cctz::parse() because we cannot distinguish overflow otherwise
  cctz::time_point<cctz::seconds> tp_seconds;
  cctz::detail::femtoseconds femtoseconds;
  if (!cctz::detail::parse(format, timestring, cctz::utc_time_zone(),
                           &tp_seconds, &femtoseconds)) {
    throw DateParseError(timestring);
  }

  // manually cast to a coarser time_point
  if (std::chrono::time_point_cast<Days>(tp_seconds) >= kTaxiInfinity ||
      tp_seconds > std::chrono::time_point_cast<decltype(tp_seconds)::duration>(
                       SystemClock::time_point::max())) {
    return SystemClock::time_point::max();
  }

  return std::chrono::time_point_cast<SystemClock::duration>(tp_seconds) +
         std::chrono::duration_cast<SystemClock::duration>(femtoseconds);
}

}  // namespace utils::datetime

USERVER_NAMESPACE_END
