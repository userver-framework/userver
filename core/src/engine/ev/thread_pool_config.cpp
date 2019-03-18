#include "thread_pool_config.hpp"

#include <yaml_config/value.hpp>

namespace engine {
namespace ev {

ThreadPoolConfig ThreadPoolConfig::ParseFromYaml(
    const formats::yaml::Value& yaml, const std::string& full_path,
    const yaml_config::VariableMapPtr& config_vars_ptr) {
  ThreadPoolConfig config;

  auto optional_threads = yaml_config::ParseOptionalUint64(
      yaml, "threads", full_path, config_vars_ptr);
  if (optional_threads) config.threads = *optional_threads;

  auto optional_thread_name = yaml_config::ParseOptionalString(
      yaml, "thread_name", full_path, config_vars_ptr);
  if (optional_thread_name)
    config.thread_name = std::move(*optional_thread_name);

  return config;
}

}  // namespace ev
}  // namespace engine
