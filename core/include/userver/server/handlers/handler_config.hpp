#pragma once

#include <optional>
#include <string>
#include <variant>

#include <userver/server/handlers/auth/handler_auth_config.hpp>
#include <userver/server/handlers/fallback_handlers.hpp>

USERVER_NAMESPACE_BEGIN

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
  std::optional<size_t> max_url_size;
  size_t max_request_size{0};
  std::optional<size_t> max_headers_size;
  size_t request_body_size_log_limit{0};
  size_t response_data_size_log_limit{0};
  std::optional<bool> parse_args_from_body;
  std::optional<auth::HandlerAuthConfig> auth;
  UrlTrailingSlashOption url_trailing_slash{UrlTrailingSlashOption::kDefault};
  std::optional<size_t> max_requests_in_flight;
  std::optional<size_t> max_requests_per_second;
  bool decompress_request{false};
  bool throttling_enabled{true};
  bool response_body_stream{false};
  std::optional<bool> set_response_server_hostname;
};

HandlerConfig Parse(const yaml_config::YamlConfig& value,
                    formats::parse::To<HandlerConfig>);

}  // namespace server::handlers

USERVER_NAMESPACE_END
