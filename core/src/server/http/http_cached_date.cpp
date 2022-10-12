#include <server/http/http_cached_date.hpp>

#include <chrono>
#include <cstring>

#include <userver/utils/datetime/wall_coarse_clock.hpp>

#include <cctz/time_zone.h>

USERVER_NAMESPACE_BEGIN

namespace server::http {

std::string_view GetCachedHttpDate() {
  static thread_local std::chrono::seconds::rep last_second = 0;
  static thread_local char last_time_string[128]{};
  static thread_local std::string_view result_view{};

  static const std::string kFormatString = "%a, %d %b %Y %H:%M:%S %Z";
  static const auto tz = cctz::utc_time_zone();

  const auto now = utils::datetime::WallCoarseClock::now();
  const auto now_seconds =
      std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
          .count();
  if (now_seconds != last_second) {
    last_second = now_seconds;

    const auto time_str = cctz::format(
        kFormatString, utils::datetime::WallCoarseClock::now(), tz);
    std::memcpy(last_time_string, time_str.c_str(), time_str.size());
    result_view = std::string_view{last_time_string, time_str.size()};
  }

  return result_view;
}

std::string GetHttpDate() {
  static const std::string kFormatString = "%a, %d %b %Y %H:%M:%S %Z";
  static const auto tz = cctz::utc_time_zone();

  return cctz::format(kFormatString, utils::datetime::WallCoarseClock::now(),
                      tz);
}

}  // namespace server::http

USERVER_NAMESPACE_END
