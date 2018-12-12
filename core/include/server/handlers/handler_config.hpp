#pragma once

#include <string>

#include <boost/optional.hpp>
#include <formats/yaml.hpp>
#include <server/handlers/auth/handler_auth_config.hpp>

#include <yaml_config/variable_map.hpp>

namespace server {
namespace handlers {

struct HandlerConfig {
  std::string path;
  std::string task_processor;
  boost::optional<std::string> method;
  boost::optional<size_t> max_url_size;
  boost::optional<size_t> max_request_size;
  boost::optional<size_t> max_headers_size;
  boost::optional<bool> parse_args_from_body;
  boost::optional<auth::HandlerAuthConfig> auth;

  static HandlerConfig ParseFromYaml(
      const formats::yaml::Node& yaml, const std::string& full_path,
      const yaml_config::VariableMapPtr& config_vars_ptr);
};

}  // namespace handlers
}  // namespace server
