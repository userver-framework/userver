#include <server/handlers/handler_config.hpp>

#include <yaml_config/value.hpp>

namespace server {
namespace handlers {
namespace {

const std::string kUrlTrailingSlashBoth = "both";
const std::string kUrlTrailingSlashStrictMatch = "strict-match";

}  // namespace

const size_t kLogRequestDataSizeDefaultLimit = 512;

UrlTrailingSlashOption Parse(const formats::yaml::Value& yaml,
                             formats::parse::To<UrlTrailingSlashOption>) {
  const auto& value = yaml.As<std::string>();
  if (value == kUrlTrailingSlashBoth) return UrlTrailingSlashOption::kBoth;
  if (value == kUrlTrailingSlashStrictMatch)
    return UrlTrailingSlashOption::kStrictMatch;
  throw std::runtime_error("can't parse UrlTrailingSlashOption from '" + value +
                           '\'');
}

HandlerConfig HandlerConfig::ParseFromYaml(
    const formats::yaml::Value& yaml, const std::string& full_path,
    const yaml_config::VariableMapPtr& config_vars_ptr) {
  HandlerConfig config;
  yaml_config::ParseInto(config.path, yaml, "path", full_path, config_vars_ptr);
  yaml_config::ParseInto(config.task_processor, yaml, "task_processor",
                         full_path, config_vars_ptr);
  yaml_config::ParseInto(config.method, yaml, "method", full_path,
                         config_vars_ptr);
  yaml_config::ParseInto(config.max_url_size, yaml, "max_url_size", full_path,
                         config_vars_ptr);
  yaml_config::ParseInto(config.max_request_size, yaml, "max_request_size",
                         full_path, config_vars_ptr);
  yaml_config::ParseInto(config.max_headers_size, yaml, "max_headers_size",
                         full_path, config_vars_ptr);
  yaml_config::ParseInto(config.parse_args_from_body, yaml,
                         "parse_args_from_body", full_path, config_vars_ptr);
  yaml_config::ParseInto(config.auth, yaml, "auth", full_path, config_vars_ptr);
  yaml_config::ParseInto(config.url_trailing_slash, yaml, "url_trailing_slash",
                         full_path, config_vars_ptr);

  boost::optional<size_t> request_body_size_log_limit;
  yaml_config::ParseInto(request_body_size_log_limit, yaml,
                         "request_body_size_log_limit", full_path,
                         config_vars_ptr);
  config.request_body_size_log_limit = request_body_size_log_limit
                                           ? *request_body_size_log_limit
                                           : kLogRequestDataSizeDefaultLimit;

  boost::optional<size_t> response_data_size_log_limit;
  yaml_config::ParseInto(response_data_size_log_limit, yaml,
                         "response_data_size_log_limit", full_path,
                         config_vars_ptr);
  config.response_data_size_log_limit = response_data_size_log_limit
                                            ? *response_data_size_log_limit
                                            : kLogRequestDataSizeDefaultLimit;

  return config;
}

}  // namespace handlers
}  // namespace server
