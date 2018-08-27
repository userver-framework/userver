#include "pool_config.hpp"

#include <yandex/taxi/userver/json_config/value.hpp>

namespace engine {
namespace coro {

PoolConfig PoolConfig::ParseFromJson(
    const Json::Value& json, const std::string& full_path,
    const json_config::VariableMapPtr& config_vars_ptr) {
  PoolConfig config;
  config.initial_size = json_config::ParseUint64(json, "initial_size",
                                                 full_path, config_vars_ptr);
  config.max_size =
      json_config::ParseUint64(json, "max_size", full_path, config_vars_ptr);
  return config;
}

}  // namespace coro
}  // namespace engine
