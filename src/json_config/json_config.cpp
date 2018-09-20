#include <json_config/json_config.hpp>

#include <json_config/value.hpp>

namespace json_config {

JsonConfig::JsonConfig(formats::json::Value json, std::string full_path,
                       VariableMapPtr config_vars_ptr)
    : json_(std::move(json)),
      full_path_(std::move(full_path)),
      config_vars_ptr_(std::move(config_vars_ptr)) {}

const formats::json::Value& JsonConfig::Json() const { return json_; }
const std::string& JsonConfig::FullPath() const { return full_path_; }
const json_config::VariableMapPtr& JsonConfig::ConfigVarsPtr() const {
  return config_vars_ptr_;
}

int JsonConfig::ParseInt(const std::string& name) const {
  return json_config::ParseInt(json_, name, full_path_, config_vars_ptr_);
}

int JsonConfig::ParseInt(const std::string& name, int dflt) const {
  return json_config::ParseOptionalInt(json_, name, full_path_,
                                       config_vars_ptr_)
      .value_or(dflt);
}

boost::optional<int> JsonConfig::ParseOptionalInt(
    const std::string& name) const {
  return json_config::ParseOptionalInt(json_, name, full_path_,
                                       config_vars_ptr_);
}

bool JsonConfig::ParseBool(const std::string& name) const {
  return json_config::ParseBool(json_, name, full_path_, config_vars_ptr_);
}

bool JsonConfig::ParseBool(const std::string& name, bool dflt) const {
  return json_config::ParseOptionalBool(json_, name, full_path_,
                                        config_vars_ptr_)
      .value_or(dflt);
}

boost::optional<bool> JsonConfig::ParseOptionalBool(
    const std::string& name) const {
  return json_config::ParseOptionalBool(json_, name, full_path_,
                                        config_vars_ptr_);
}

uint64_t JsonConfig::ParseUint64(const std::string& name) const {
  return json_config::ParseUint64(json_, name, full_path_, config_vars_ptr_);
}

uint64_t JsonConfig::ParseUint64(const std::string& name, uint64_t dflt) const {
  return json_config::ParseOptionalUint64(json_, name, full_path_,
                                          config_vars_ptr_)
      .value_or(dflt);
}

boost::optional<uint64_t> JsonConfig::ParseOptionalUint64(
    const std::string& name) const {
  return json_config::ParseOptionalUint64(json_, name, full_path_,
                                          config_vars_ptr_);
}

std::string JsonConfig::ParseString(const std::string& name) const {
  return json_config::ParseString(json_, name, full_path_, config_vars_ptr_);
}

std::string JsonConfig::ParseString(const std::string& name,
                                    const std::string& dflt) const {
  return json_config::ParseOptionalString(json_, name, full_path_,
                                          config_vars_ptr_)
      .value_or(dflt);
}

boost::optional<std::string> JsonConfig::ParseOptionalString(
    const std::string& name) const {
  return json_config::ParseOptionalString(json_, name, full_path_,
                                          config_vars_ptr_);
}

}  // namespace json_config
