#pragma once

#include <string>

#include <json/json.h>

#include <yandex/taxi/userver/json_config/variable_map.hpp>

namespace engine {
namespace coro {

struct PoolConfig {
  size_t initial_size = 1000;
  size_t max_size = 10000;

  static PoolConfig ParseFromJson(
      const Json::Value& json, const std::string& full_path,
      const json_config::VariableMapPtr& config_vars_ptr);
};

}  // namespace coro
}  // namespace engine
