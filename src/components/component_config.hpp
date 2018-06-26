#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include <json/value.h>

#include <json_config/json_config.hpp>
#include <json_config/variable_map.hpp>

namespace components {

class ComponentConfig : public json_config::JsonConfig {
 public:
  ComponentConfig(Json::Value json, std::string full_path,
                  json_config::VariableMapPtr config_vars_ptr);

  const std::string& Name() const;

  static ComponentConfig ParseFromJson(
      const Json::Value& json, const std::string& full_path,
      const json_config::VariableMapPtr& config_vars_ptr);

 private:
  std::string name_;
};

using ComponentConfigMap =
    std::unordered_map<std::string, const ComponentConfig&>;

}  // namespace components
