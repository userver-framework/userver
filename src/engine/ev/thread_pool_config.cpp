#include "thread_pool_config.hpp"

#include <json_config/value.hpp>

namespace engine {
namespace ev {

ThreadPoolConfig ThreadPoolConfig::ParseFromJson(
    const formats::json::Value& json, const std::string& full_path,
    const json_config::VariableMapPtr& config_vars_ptr) {
  ThreadPoolConfig config;

  auto optional_threads = json_config::ParseOptionalUint64(
      json, "threads", full_path, config_vars_ptr);
  if (optional_threads) config.threads = *optional_threads;

  auto optional_thread_name = json_config::ParseOptionalString(
      json, "thread_name", full_path, config_vars_ptr);
  if (optional_thread_name)
    config.thread_name = std::move(*optional_thread_name);

  return config;
}

}  // namespace ev
}  // namespace engine
