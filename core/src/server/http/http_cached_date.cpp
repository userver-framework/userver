#include <server/http/http_cached_date.hpp>

#include <cstring>

#include <userver/utils/assert.hpp>
#include <userver/utils/datetime/wall_coarse_clock.hpp>

#include <cctz/time_zone.h>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace impl {

std::string MakeHttpDate(std::chrono::system_clock::time_point date) {
  static const std::string kFormatString = "%a, %d %b %Y %H:%M:%S %Z";
  static const auto tz = cctz::utc_time_zone();

  return cctz::format(kFormatString, date, tz);
}

std::string_view GetCachedDate() {
  constexpr size_t kMaxDateHeaderLength = 128;

  static thread_local std::chrono::seconds::rep last_second = 0;
  static thread_local char last_time_string[kMaxDateHeaderLength]{};
  static thread_local std::string_view result_view{};

  const auto now = utils::datetime::WallCoarseClock::now();
  const auto now_seconds =
      std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
          .count();
  if (now_seconds != last_second) {
    last_second = now_seconds;

    const auto time_str = impl::MakeHttpDate(now);
    // this should never fire, but is left for some convenience
    UASSERT(time_str.size() <= kMaxDateHeaderLength);

    std::memcpy(last_time_string, time_str.c_str(), time_str.size());
    result_view = std::string_view{last_time_string, time_str.size()};
  }

  return result_view;
}

}  // namespace impl

void AppendCachedDate(std::string& header) {
  header.append(impl::GetCachedDate());
}

}  // namespace server::http

USERVER_NAMESPACE_END
