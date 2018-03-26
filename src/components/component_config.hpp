#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include <json/value.h>

#include <json_config/variable_map.hpp>

namespace components {

class ComponentConfig {
 public:
  ComponentConfig(Json::Value json, std::string full_path,
                  json_config::VariableMapPtr config_vars_ptr);

  const std::string& Name() const;

  const Json::Value& Json() const;
  const std::string& FullPath() const;
  const json_config::VariableMapPtr& ConfigVarsPtr() const;

  int ParseInt(const std::string& name) const;
  int ParseInt(const std::string& name, int dflt) const;
  bool ParseBool(const std::string& name) const;
  bool ParseBool(const std::string& name, bool dflt) const;
  uint64_t ParseUint64(const std::string& name) const;
  uint64_t ParseUint64(const std::string& name, uint64_t dflt) const;
  std::string ParseString(const std::string& name) const;
  std::string ParseString(const std::string& name,
                          const std::string& dflt) const;

  static ComponentConfig ParseFromJson(
      const Json::Value& json, const std::string& full_path,
      const json_config::VariableMapPtr& config_vars_ptr);

 private:
  Json::Value json_;
  std::string full_path_;
  json_config::VariableMapPtr config_vars_ptr_;

  std::string name_;
};

using ComponentConfigMap =
    std::unordered_map<std::string, const ComponentConfig&>;

}  // namespace components
