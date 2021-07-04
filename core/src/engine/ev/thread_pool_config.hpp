#pragma once

#include <string>

#include <userver/formats/yaml.hpp>
#include <userver/yaml_config/yaml_config.hpp>

namespace engine {
namespace ev {

struct ThreadPoolConfig {
  size_t threads = 2;
  std::string thread_name = "event-worker";
};

ThreadPoolConfig Parse(const yaml_config::YamlConfig& value,
                       formats::parse::To<ThreadPoolConfig>);

}  // namespace ev
}  // namespace engine
