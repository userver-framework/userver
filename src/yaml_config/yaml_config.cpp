#include <yaml_config/yaml_config.hpp>

#include <yaml_config/value.hpp>

namespace yaml_config {

YamlConfig::YamlConfig(formats::yaml::Node yaml, std::string full_path,
                       VariableMapPtr config_vars_ptr)
    : yaml_(std::move(yaml)),
      full_path_(std::move(full_path)),
      config_vars_ptr_(std::move(config_vars_ptr)) {}

const formats::yaml::Node& YamlConfig::Yaml() const { return yaml_; }
const std::string& YamlConfig::FullPath() const { return full_path_; }
const yaml_config::VariableMapPtr& YamlConfig::ConfigVarsPtr() const {
  return config_vars_ptr_;
}

int YamlConfig::ParseInt(const std::string& name) const {
  return yaml_config::ParseInt(yaml_, name, full_path_, config_vars_ptr_);
}

int YamlConfig::ParseInt(const std::string& name, int dflt) const {
  return yaml_config::ParseOptionalInt(yaml_, name, full_path_,
                                       config_vars_ptr_)
      .value_or(dflt);
}

boost::optional<int> YamlConfig::ParseOptionalInt(
    const std::string& name) const {
  return yaml_config::ParseOptionalInt(yaml_, name, full_path_,
                                       config_vars_ptr_);
}

bool YamlConfig::ParseBool(const std::string& name) const {
  return yaml_config::ParseBool(yaml_, name, full_path_, config_vars_ptr_);
}

bool YamlConfig::ParseBool(const std::string& name, bool dflt) const {
  return yaml_config::ParseOptionalBool(yaml_, name, full_path_,
                                        config_vars_ptr_)
      .value_or(dflt);
}

boost::optional<bool> YamlConfig::ParseOptionalBool(
    const std::string& name) const {
  return yaml_config::ParseOptionalBool(yaml_, name, full_path_,
                                        config_vars_ptr_);
}

uint64_t YamlConfig::ParseUint64(const std::string& name) const {
  return yaml_config::ParseUint64(yaml_, name, full_path_, config_vars_ptr_);
}

uint64_t YamlConfig::ParseUint64(const std::string& name, uint64_t dflt) const {
  return yaml_config::ParseOptionalUint64(yaml_, name, full_path_,
                                          config_vars_ptr_)
      .value_or(dflt);
}

boost::optional<uint64_t> YamlConfig::ParseOptionalUint64(
    const std::string& name) const {
  return yaml_config::ParseOptionalUint64(yaml_, name, full_path_,
                                          config_vars_ptr_);
}

std::string YamlConfig::ParseString(const std::string& name) const {
  return yaml_config::ParseString(yaml_, name, full_path_, config_vars_ptr_);
}

std::string YamlConfig::ParseString(const std::string& name,
                                    const std::string& dflt) const {
  return yaml_config::ParseOptionalString(yaml_, name, full_path_,
                                          config_vars_ptr_)
      .value_or(dflt);
}

boost::optional<std::string> YamlConfig::ParseOptionalString(
    const std::string& name) const {
  return yaml_config::ParseOptionalString(yaml_, name, full_path_,
                                          config_vars_ptr_);
}

}  // namespace yaml_config
