#include <userver/clients/http/impl/config.hpp>

#include <string_view>

#include <userver/dynamic_config/value.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::impl {
namespace {

void ParseTokenBucketSettings(const formats::json::Value& settings,
                              size_t& limit, std::chrono::microseconds& rate,
                              std::string_view limit_key,
                              std::string_view per_second_key) {
  limit = settings[limit_key].As<size_t>(ThrottleConfig::kNoLimit);
  if (limit == 0) limit = ThrottleConfig::kNoLimit;

  if (limit != ThrottleConfig::kNoLimit) {
    const auto per_second = settings[per_second_key].As<size_t>(0);
    if (per_second) {
      rate = std::chrono::seconds{1};
      rate /= per_second;
    } else {
      limit = ThrottleConfig::kNoLimit;
    }
  }
}

DeadlinePropagationConfig ParseDeadlinePropagationConfig(
    const yaml_config::YamlConfig& value) {
  DeadlinePropagationConfig result;
  result.update_header =
      value["set-deadline-propagation-header"].As<bool>(result.update_header);
  return result;
}

}  // namespace

ClientSettings Parse(const yaml_config::YamlConfig& value,
                     formats::parse::To<ClientSettings>) {
  ClientSettings result;
  result.thread_name_prefix =
      value["thread-name-prefix"].As<std::string>(result.thread_name_prefix);
  result.io_threads = value["threads"].As<size_t>(result.io_threads);
  result.defer_events = value["defer-events"].As<bool>(result.defer_events);
  result.deadline_propagation = ParseDeadlinePropagationConfig(value);
  return result;
}

ThrottleConfig Parse(const formats::json::Value& value,
                     formats::parse::To<ThrottleConfig>) {
  ThrottleConfig result;
  ParseTokenBucketSettings(value, result.http_connect_limit,
                           result.http_connect_rate, "http-limit",
                           "http-per-second");
  ParseTokenBucketSettings(value, result.https_connect_limit,
                           result.https_connect_rate, "https-limit",
                           "https-per-second");
  ParseTokenBucketSettings(value, result.per_host_connect_limit,
                           result.per_host_connect_rate, "per-host-limit",
                           "per-host-per-second");
  return result;
}

Config ParseConfig(const dynamic_config::DocsMap& docs_map) {
  Config result;
  result.connection_pool_size =
      docs_map.Get("HTTP_CLIENT_CONNECTION_POOL_SIZE").As<std::size_t>();
  result.proxy = docs_map.Get("USERVER_HTTP_PROXY").As<std::string>();
  result.throttle =
      docs_map.Get("HTTP_CLIENT_CONNECT_THROTTLE").As<ThrottleConfig>();
  return result;
}

}  // namespace clients::http::impl

USERVER_NAMESPACE_END
