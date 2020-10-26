#pragma once

#include <optional>
#include <string>
#include <variant>

#include <server/handlers/auth/handler_auth_config.hpp>
#include <server/handlers/fallback_handlers.hpp>

namespace server {
namespace handlers {

/// Defines matching behavior for paths with trailing slashes.
enum class UrlTrailingSlashOption {
  kBoth,         ///< ignore trailing slashes when matching paths
  kStrictMatch,  ///< require exact match for trailing slashes in paths

  kDefault = kBoth,
};

struct HandlerConfig {
  std::variant<std::string, FallbackHandler> path;
  std::string task_processor;
  std::optional<std::string> method;
  std::optional<size_t> max_url_size;
  std::optional<size_t> max_request_size;
  std::optional<size_t> max_headers_size;
  size_t request_body_size_log_limit{0};
  size_t response_data_size_log_limit{0};
  std::optional<bool> parse_args_from_body;
  std::optional<auth::HandlerAuthConfig> auth;
  std::optional<UrlTrailingSlashOption> url_trailing_slash;
  std::optional<size_t> max_requests_in_flight;
  std::optional<size_t> max_requests_per_second;
};

HandlerConfig Parse(const yaml_config::YamlConfig& value,
                    formats::parse::To<HandlerConfig>);

}  // namespace handlers
}  // namespace server
