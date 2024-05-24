#pragma once

#include <string>

#include <userver/formats/yaml.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::coro {

struct PoolConfig {
  std::size_t initial_size = 1000;
  std::size_t max_size = 4000;
  std::size_t stack_size = 256 * 1024ULL;
};

PoolConfig Parse(const yaml_config::YamlConfig& value,
                 formats::parse::To<PoolConfig>);

}  // namespace engine::coro

USERVER_NAMESPACE_END
