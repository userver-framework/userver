#pragma once

#include <string>

#include <formats/yaml.hpp>
#include <yaml_config/yaml_config.hpp>

namespace engine {
namespace coro {

struct PoolConfig {
  size_t initial_size = 1000;
  size_t max_size = 10000;
};

PoolConfig Parse(const yaml_config::YamlConfig& value,
                 formats::parse::To<PoolConfig>);

}  // namespace coro
}  // namespace engine
