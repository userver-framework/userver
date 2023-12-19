#include <server/http/http_cached_date.hpp>

#include <cstring>

#include <cctz/time_zone.h>

#include <userver/compiler/thread_local.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/datetime/wall_coarse_clock.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace impl {

std::string MakeHttpDate(std::chrono::system_clock::time_point date) {
  static const std::string kFormatString = "%a, %d %b %Y %H:%M:%S %Z";
  static const auto tz = cctz::utc_time_zone();

  return cctz::format(kFormatString, date, tz);
}

constexpr size_t kMaxDateHeaderLength = 128;

struct LocalTimeCache final {
  std::chrono::seconds last_second{0};
  std::size_t last_time_string_size{};
  char last_time_string[kMaxDateHeaderLength]{};
};

std::string_view GetCachedDate() {
  static compiler::ThreadLocal local_cache = [] { return LocalTimeCache{}; };
  auto cache = local_cache.Use();

  const auto now = utils::datetime::WallCoarseClock::now();
  const auto now_seconds =
      std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
  if (now_seconds != cache->last_second) {
    cache->last_second = now_seconds;

    const auto time_str = impl::MakeHttpDate(now);
    // this should never fire, but is left for some convenience
    UASSERT(time_str.size() <= kMaxDateHeaderLength);

    std::memcpy(cache->last_time_string, time_str.c_str(), time_str.size());
    cache->last_time_string_size = time_str.size();
  }

  return std::string_view{cache->last_time_string,
                          cache->last_time_string_size};
}

}  // namespace impl

void AppendCachedDate(std::string& header) {
  header.append(impl::GetCachedDate());
}

}  // namespace server::http

USERVER_NAMESPACE_END
