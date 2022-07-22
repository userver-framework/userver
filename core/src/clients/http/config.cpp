#include <clients/http/config.hpp>

#include <string_view>

#include <userver/dynamic_config/value.hpp>
#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

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

EnforceTaskDeadlineConfig Parse(const formats::json::Value& value,
                                formats::parse::To<EnforceTaskDeadlineConfig>) {
  EnforceTaskDeadlineConfig result;
  result.cancel_request = value["cancel-request"].As<bool>();
  result.update_timeout = value["update-timeout"].As<bool>();
  return result;
}

Config::Config(const dynamic_config::DocsMap& docs_map)
    : connection_pool_size(
          docs_map.Get("HTTP_CLIENT_CONNECTION_POOL_SIZE").As<std::size_t>()),
      enforce_task_deadline(docs_map.Get("HTTP_CLIENT_ENFORCE_TASK_DEADLINE")
                                .As<EnforceTaskDeadlineConfig>()),
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

USERVER_NAMESPACE_END
