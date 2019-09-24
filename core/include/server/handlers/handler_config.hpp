#pragma once

#include <string>

#include <boost/optional.hpp>
#include <formats/yaml.hpp>
#include <server/handlers/auth/handler_auth_config.hpp>

#include <yaml_config/variable_map.hpp>

namespace server {
namespace handlers {

/// Defines matching behavior for paths with trailing slashes.
enum class UrlTrailingSlashOption {
  kBoth,         ///< ignore trailing slashes when matching paths
  kStrictMatch,  ///< require exact match for trailing slashes in paths
};

struct HandlerConfig {
  std::string path;
  std::string task_processor;
  boost::optional<std::string> method;
  boost::optional<size_t> max_url_size;
  boost::optional<size_t> max_request_size;
  boost::optional<size_t> max_headers_size;
  size_t request_body_size_log_limit{0};
  size_t response_data_size_log_limit{0};
  boost::optional<bool> parse_args_from_body;
  boost::optional<auth::HandlerAuthConfig> auth;
  boost::optional<UrlTrailingSlashOption> url_trailing_slash;
  boost::optional<size_t> max_requests_in_flight;

  static HandlerConfig ParseFromYaml(
      const formats::yaml::Value& yaml, const std::string& full_path,
      const yaml_config::VariableMapPtr& config_vars_ptr);
};

}  // namespace handlers
}  // namespace server
