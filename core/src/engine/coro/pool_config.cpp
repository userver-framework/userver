#include "pool_config.hpp"

#include <yaml_config/value.hpp>

namespace engine {
namespace coro {

PoolConfig PoolConfig::ParseFromYaml(
    const formats::yaml::Value& yaml, const std::string& full_path,
    const yaml_config::VariableMapPtr& config_vars_ptr) {
  PoolConfig config;
  config.initial_size = yaml_config::ParseUint64(yaml, "initial_size",
                                                 full_path, config_vars_ptr);
  config.max_size =
      yaml_config::ParseUint64(yaml, "max_size", full_path, config_vars_ptr);
  return config;
}

}  // namespace coro
}  // namespace engine
