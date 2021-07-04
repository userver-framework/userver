#pragma once

#include <string>

#include <userver/formats/yaml.hpp>
#include <userver/yaml_config/yaml_config.hpp>

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
