#include <utils/datetime/from_string_saturating.hpp>

#include <boost/algorithm/string/predicate.hpp>

#include <utils/assert.hpp>
#include <utils/datetime.hpp>

namespace utils::datetime {

namespace {

bool TimePointOverflows(const std::string& timestring) {
  // TODO: in C++20 replace with std::chrono::years
  using Years = std::chrono::duration<std::int64_t,
                                      std::ratio<146097LL * 24 * 60 * 60, 400>>;

  constexpr int kMaxRfc3339Year = 9999;
  constexpr auto kMaxPlatformYear =
      std::chrono::duration_cast<Years>(
          std::chrono::system_clock::time_point::duration::max())
          .count() +
      1970;

  constexpr auto kMaxYear =
      (kMaxRfc3339Year < kMaxPlatformYear ? kMaxRfc3339Year
                                          : static_cast<int>(kMaxPlatformYear));

  const auto years = std::stoll(timestring);
  return years >= kMaxYear;
}

}  // namespace

std::chrono::system_clock::time_point FromRfc3339StringSaturating(
    const std::string& timestring) {
  if (TimePointOverflows(timestring)) {
    return std::chrono::system_clock::time_point::max();
  }

  return datetime::Stringtime(timestring, kDefaultTimezone, kRfc3339Format);
}

std::chrono::system_clock::time_point FromStringSaturating(
    const std::string& timestring, const std::string& format) {
  YTX_INVARIANT(boost::starts_with(format, "%Y"),
                "`format` should start with `%Y` in FromStringSaturating");

  if (TimePointOverflows(timestring)) {
    return std::chrono::system_clock::time_point::max();
  }

  return datetime::Stringtime(timestring, kDefaultTimezone, format);
}

}  // namespace utils::datetime
