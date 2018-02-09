#include "pool_config.hpp"

#include <json_config/value.hpp>

namespace engine {
namespace coro {

PoolConfig PoolConfig::ParseFromJson(
    const Json::Value& json, const std::string& full_path,
    const json_config::VariableMap& config_vars) {
  PoolConfig config;
  config.initial_size =
      json_config::ParseUint64(json, "initial_size", full_path, config_vars);
  config.max_size =
      json_config::ParseUint64(json, "max_size", full_path, config_vars);
  return config;
}

}  // namespace coro
}  // namespace engine
