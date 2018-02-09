#pragma once

#include <string>

#include <json/json.h>

#include <json_config/variable_map.hpp>

namespace engine {
namespace coro {

struct PoolConfig {
  size_t initial_size = 1000;
  size_t max_size = 10000;

  static PoolConfig ParseFromJson(const Json::Value& json,
                                  const std::string& full_path,
                                  const json_config::VariableMap& config_vars);
};

}  // namespace coro
}  // namespace engine
