#include <server/handlers/handler_config.hpp>

#include <fmt/format.h>
#include <formats/parse/common_containers.hpp>
#include <yaml_config/value.hpp>

namespace server::handlers {
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

FallbackHandler Parse(const formats::yaml::Value& yaml,
                      formats::parse::To<FallbackHandler>) {
  const auto& value = yaml.As<std::string>();
  return FallbackHandlerFromString(value);
}

HandlerConfig HandlerConfig::ParseFromYaml(
    const formats::yaml::Value& yaml, const std::string& full_path,
    const yaml_config::VariableMapPtr& config_vars_ptr) {
  HandlerConfig config;

  config.path = [&yaml, &full_path,
                 &config_vars_ptr]() -> decltype(config.path) {
    std::optional<std::string> opt_path;
    std::optional<FallbackHandler> opt_fallback;
    yaml_config::ParseInto(opt_path, yaml, "path", full_path, config_vars_ptr);
    yaml_config::ParseInto(opt_fallback, yaml, "as_fallback", full_path,
                           config_vars_ptr);
    if (!opt_path == !opt_fallback)
      throw std::runtime_error(
          fmt::format("Expected 'path' or 'as_fallback' at {}, but {} provided",
                      full_path, (!opt_path) ? "no one" : "both"));

    if (opt_path)
      return *opt_path;
    else
      return *opt_fallback;
  }();

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
  yaml_config::ParseInto(config.max_requests_in_flight, yaml,
                         "max_requests_in_flight", full_path, config_vars_ptr);

  std::optional<size_t> request_body_size_log_limit;
  yaml_config::ParseInto(request_body_size_log_limit, yaml,
                         "request_body_size_log_limit", full_path,
                         config_vars_ptr);
  config.request_body_size_log_limit = request_body_size_log_limit
                                           ? *request_body_size_log_limit
                                           : kLogRequestDataSizeDefaultLimit;

  std::optional<size_t> response_data_size_log_limit;
  yaml_config::ParseInto(response_data_size_log_limit, yaml,
                         "response_data_size_log_limit", full_path,
                         config_vars_ptr);
  config.response_data_size_log_limit = response_data_size_log_limit
                                            ? *response_data_size_log_limit
                                            : kLogRequestDataSizeDefaultLimit;

  yaml_config::ParseInto(config.max_requests_per_second, yaml,
                         "max_requests_per_second", full_path, config_vars_ptr);

  if (config.max_requests_per_second &&
      config.max_requests_per_second.value() <= 0) {
    throw std::runtime_error(
        "max_requests_per_second should be greater than 0, current value is " +
        std::to_string(config.max_requests_per_second.value()));
  }

  return config;
}

}  // namespace server::handlers
