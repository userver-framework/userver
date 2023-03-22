#pragma once

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <userver/server/handlers/auth/handler_auth_config.hpp>
#include <userver/server/handlers/fallback_handlers.hpp>
#include <userver/server/request/request_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace server {
struct ServerConfig;
}  // namespace server

namespace server::handlers {

/// Defines matching behavior for paths with trailing slashes.
enum class UrlTrailingSlashOption {
  kBoth,         ///< ignore trailing slashes when matching paths
  kStrictMatch,  ///< require exact match for trailing slashes in paths

  kDefault = kBoth,
};

struct HandlerConfig {
  std::variant<std::string, FallbackHandler> path;
  std::string task_processor;
  std::string method;
  request::HttpRequestConfig request_config{};
  size_t request_body_size_log_limit{0};
  size_t response_data_size_log_limit{0};
  std::optional<auth::HandlerAuthConfig> auth;
  UrlTrailingSlashOption url_trailing_slash{UrlTrailingSlashOption::kDefault};
  std::optional<size_t> max_requests_in_flight;
  std::optional<size_t> max_requests_per_second;
  bool decompress_request{false};
  bool throttling_enabled{true};
  bool response_body_stream{false};
  std::optional<bool> set_response_server_hostname;
  bool set_tracing_headers{true};
};

HandlerConfig ParseHandlerConfigsWithDefaults(
    const yaml_config::YamlConfig& value,
    const server::ServerConfig& server_config, bool is_monitor = false);

}  // namespace server::handlers

USERVER_NAMESPACE_END
