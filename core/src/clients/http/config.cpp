#include <clients/http/config.hpp>

#include <string_view>

#include <formats/json/value.hpp>

namespace clients::http {
namespace {

void ParseTokenBucketSettings(const formats::json::Value& settings,
                              size_t& limit, std::chrono::microseconds& rate,
                              std::string_view limit_key,
                              std::string_view per_second_key) {
  limit = settings[limit_key].As<size_t>(Config::kNoLimit);
  if (limit == 0) limit = Config::kNoLimit;

  if (limit != Config::kNoLimit) {
    const auto per_second = settings[per_second_key].As<size_t>(0);
    if (per_second) {
      rate = std::chrono::seconds{1};
      rate /= per_second;
    } else {
      limit = Config::kNoLimit;
    }
  }
}

}  // namespace

Config::Config(const DocsMap& docs_map)
    : connection_pool_size("HTTP_CLIENT_CONNECTION_POOL_SIZE", docs_map),
      http_connect_throttle_limit(kNoLimit),
      http_connect_throttle_rate(0),
      https_connect_throttle_limit(kNoLimit),
      https_connect_throttle_rate(0),
      per_host_connect_throttle_limit(kNoLimit),
      per_host_connect_throttle_rate(0),
      proxy(docs_map.Get("USERVER_HTTP_PROXY").As<std::string>()) {
  const auto throttle_settings = docs_map.Get("HTTP_CLIENT_CONNECT_THROTTLE");
  ParseTokenBucketSettings(throttle_settings, http_connect_throttle_limit,
                           http_connect_throttle_rate, "http-limit",
                           "http-per-second");
  ParseTokenBucketSettings(throttle_settings, https_connect_throttle_limit,
                           https_connect_throttle_rate, "https-limit",
                           "https-per-second");
  ParseTokenBucketSettings(throttle_settings, per_host_connect_throttle_limit,
                           per_host_connect_throttle_rate, "per-host-limit",
                           "per-host-per-second");
}

}  // namespace clients::http
