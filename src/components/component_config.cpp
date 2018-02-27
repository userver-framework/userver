#include "component_config.hpp"

#include <stdexcept>

#include <json_config/value.hpp>

namespace components {

ComponentConfig::ComponentConfig(Json::Value json, std::string full_path,
                                 json_config::VariableMapPtr config_vars_ptr)
    : json_(std::move(json)),
      full_path_(std::move(full_path)),
      config_vars_ptr_(std::move(config_vars_ptr)) {
  name_ = json_config::ParseString(json_, "name", full_path_, config_vars_ptr_);
}

const std::string& ComponentConfig::Name() const { return name_; }

const Json::Value& ComponentConfig::Json() const { return json_; }
const std::string& ComponentConfig::FullPath() const { return full_path_; }
const json_config::VariableMapPtr& ComponentConfig::ConfigVarsPtr() const {
  return config_vars_ptr_;
}

int ComponentConfig::ParseInt(const std::string& name) const {
  return json_config::ParseInt(json_, name, full_path_, config_vars_ptr_);
}

int ComponentConfig::ParseInt(const std::string& name, int dflt) const {
  return json_config::ParseOptionalInt(json_, name, full_path_,
                                       config_vars_ptr_)
      .value_or(dflt);
}

bool ComponentConfig::ParseBool(const std::string& name) const {
  return json_config::ParseBool(json_, name, full_path_, config_vars_ptr_);
}

bool ComponentConfig::ParseBool(const std::string& name, bool dflt) const {
  return json_config::ParseOptionalBool(json_, name, full_path_,
                                        config_vars_ptr_)
      .value_or(dflt);
}

uint64_t ComponentConfig::ParseUint64(const std::string& name) const {
  return json_config::ParseUint64(json_, name, full_path_, config_vars_ptr_);
}

uint64_t ComponentConfig::ParseUint64(const std::string& name,
                                      uint64_t dflt) const {
  return json_config::ParseOptionalUint64(json_, name, full_path_,
                                          config_vars_ptr_)
      .value_or(dflt);
}

std::string ComponentConfig::ParseString(const std::string& name) const {
  return json_config::ParseString(json_, name, full_path_, config_vars_ptr_);
}

std::string ComponentConfig::ParseString(const std::string& name,
                                         const std::string& dflt) const {
  return json_config::ParseOptionalString(json_, name, full_path_,
                                          config_vars_ptr_)
      .value_or(dflt);
}

ComponentConfig ComponentConfig::ParseFromJson(
    const Json::Value& json, const std::string& full_path,
    const json_config::VariableMapPtr& config_vars_ptr) {
  return {json, full_path, config_vars_ptr};
}

}  // namespace components
