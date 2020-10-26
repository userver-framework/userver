#include "pool_config.hpp"

namespace engine::coro {

PoolConfig Parse(const yaml_config::YamlConfig& value,
                 formats::parse::To<PoolConfig>) {
  PoolConfig config;
  config.initial_size = value["initial_size"].As<size_t>();
  config.max_size = value["max_size"].As<size_t>();
  return config;
}

}  // namespace engine::coro
