#include "event_thread_pool_config.hpp"

#include <json_config/value.hpp>

namespace components {

EventThreadPoolConfig EventThreadPoolConfig::ParseFromJson(
    const Json::Value& json, const std::string& full_path,
    const json_config::VariableMapPtr& config_vars_ptr) {
  EventThreadPoolConfig config;
  config.name =
      json_config::ParseString(json, "name", full_path, config_vars_ptr);

  auto optional_threads = json_config::ParseOptionalUint64(
      json, "threads", full_path, config_vars_ptr);
  if (optional_threads) config.threads = *optional_threads;

  auto optional_thread_name = json_config::ParseOptionalString(
      json, "thread_name", full_path, config_vars_ptr);
  config.thread_name = optional_thread_name.get_value_or(config.name);

  return config;
}

}  // namespace components
