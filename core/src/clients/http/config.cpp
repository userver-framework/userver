#include <clients/http/config.hpp>

#include <formats/json/value.hpp>

namespace clients::http {
namespace {

void ParseTokenBucketSettings(const formats::json::Value& settings,
                              size_t& max_size,
                              std::chrono::microseconds& update_interval,
                              const std::string& new_max_size_key,
                              const std::string& old_max_size_key,
                              const std::string& per_second_key,
                              const std::string& update_interval_ms_key) {
  max_size = settings[new_max_size_key].As<size_t>(
      settings[old_max_size_key].As<size_t>(Config::kNoLimit));
  if (max_size == 0) max_size = Config::kNoLimit;

  if (max_size != Config::kNoLimit) {
    const auto ps_value = settings[per_second_key];
    if (!ps_value.IsMissing()) {
      const auto per_second = ps_value.As<size_t>(0);
      if (per_second) {
        update_interval = std::chrono::seconds{1};
        update_interval /= per_second;
      } else {
        max_size = Config::kNoLimit;
      }
    } else {
      update_interval = std::chrono::milliseconds{
          settings[update_interval_ms_key].As<size_t>()};
    }
  }
}

}  // namespace

Config::Config(const DocsMap& docs_map)
    : connection_pool_size("HTTP_CLIENT_CONNECTION_POOL_SIZE", docs_map),
      http_connect_throttle_max_size(kNoLimit),
      http_connect_throttle_update_interval(0),
      https_connect_throttle_max_size(kNoLimit),
      https_connect_throttle_update_interval(0) {
  const auto throttle_settings = docs_map.Get("HTTP_CLIENT_CONNECT_THROTTLE");
  ParseTokenBucketSettings(throttle_settings, http_connect_throttle_max_size,
                           http_connect_throttle_update_interval, "http-limit",
                           "http-max-size", "http-per-second",
                           "http-token-update-interval-ms");
  ParseTokenBucketSettings(throttle_settings, https_connect_throttle_max_size,
                           https_connect_throttle_update_interval,
                           "https-limit", "max-size", "https-per-second",
                           "token-update-interval-ms");
}

}  // namespace clients::http
