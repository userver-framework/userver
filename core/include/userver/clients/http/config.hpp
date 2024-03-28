#pragma once

#include <chrono>
#include <string>

#include <userver/dynamic_config/fwd.hpp>
#include <userver/formats/json_fwd.hpp>
#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {
class TracingManagerBase;
}  // namespace tracing

namespace server::http {
class HeadersPropagator;
}  // namespace server::http

namespace clients::http {

struct DeadlinePropagationConfig {
  bool update_header{true};
};

enum class CancellationPolicy {
  kIgnore,
  kCancel,
};

CancellationPolicy Parse(yaml_config::YamlConfig value,
                         formats::parse::To<CancellationPolicy>);

// Static config
struct ClientSettings final {
  std::string thread_name_prefix{};
  size_t io_threads{8};
  bool defer_events{false};
  DeadlinePropagationConfig deadline_propagation{};
  const tracing::TracingManagerBase* tracing_manager{nullptr};
  const server::http::HeadersPropagator* headers_propagator{nullptr};
  CancellationPolicy cancellation_policy{CancellationPolicy::kCancel};
};

ClientSettings Parse(const yaml_config::YamlConfig& value,
                     formats::parse::To<ClientSettings>);

}  // namespace clients::http

namespace clients::http::impl {

struct ThrottleConfig final {
  static constexpr size_t kNoLimit = -1;

  std::size_t http_connect_limit{kNoLimit};
  std::chrono::microseconds http_connect_rate{0};
  std::size_t https_connect_limit{kNoLimit};
  std::chrono::microseconds https_connect_rate{0};
  std::size_t per_host_connect_limit{kNoLimit};
  std::chrono::microseconds per_host_connect_rate{0};
};

ThrottleConfig Parse(const formats::json::Value& value,
                     formats::parse::To<ThrottleConfig>);

// Dynamic config
struct Config final {
  static constexpr std::size_t kDefaultConnectionPoolSize = 10000;

  std::size_t connection_pool_size{kDefaultConnectionPoolSize};
  std::string proxy;
  ThrottleConfig throttle;
};

Config ParseConfig(const dynamic_config::DocsMap& docs_map);

}  // namespace clients::http::impl

USERVER_NAMESPACE_END
