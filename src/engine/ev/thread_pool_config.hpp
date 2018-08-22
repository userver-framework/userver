#pragma once

#include <string>

#include <json/value.h>

#include <yandex/taxi/userver/json_config/variable_map.hpp>

namespace engine {
namespace ev {

struct ThreadPoolConfig {
  size_t threads = 2;

  static ThreadPoolConfig ParseFromJson(
      const Json::Value& json, const std::string& full_path,
      const json_config::VariableMapPtr& config_vars_ptr);
};

}  // namespace ev
}  // namespace engine
