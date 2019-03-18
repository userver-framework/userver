#include <yaml_config/yaml_config.hpp>

#include <utils/string_to_duration.hpp>

namespace yaml_config {

YamlConfig::YamlConfig(formats::yaml::Value yaml, std::string full_path,
                       VariableMapPtr config_vars_ptr)
    : yaml_(std::move(yaml)),
      full_path_(std::move(full_path)),
      config_vars_ptr_(std::move(config_vars_ptr)) {}

const formats::yaml::Value& YamlConfig::Yaml() const { return yaml_; }
const std::string& YamlConfig::FullPath() const { return full_path_; }
const yaml_config::VariableMapPtr& YamlConfig::ConfigVarsPtr() const {
  return config_vars_ptr_;
}

int YamlConfig::ParseInt(const std::string& name) const {
  return yaml_config::ParseInt(yaml_, name, full_path_, config_vars_ptr_);
}

int YamlConfig::ParseInt(const std::string& name, int default_value) const {
  return yaml_config::ParseOptionalInt(yaml_, name, full_path_,
                                       config_vars_ptr_)
      .value_or(default_value);
}

boost::optional<int> YamlConfig::ParseOptionalInt(
    const std::string& name) const {
  return yaml_config::ParseOptionalInt(yaml_, name, full_path_,
                                       config_vars_ptr_);
}

bool YamlConfig::ParseBool(const std::string& name) const {
  return yaml_config::ParseBool(yaml_, name, full_path_, config_vars_ptr_);
}

bool YamlConfig::ParseBool(const std::string& name, bool default_value) const {
  return yaml_config::ParseOptionalBool(yaml_, name, full_path_,
                                        config_vars_ptr_)
      .value_or(default_value);
}

boost::optional<bool> YamlConfig::ParseOptionalBool(
    const std::string& name) const {
  return yaml_config::ParseOptionalBool(yaml_, name, full_path_,
                                        config_vars_ptr_);
}

uint64_t YamlConfig::ParseUint64(const std::string& name) const {
  return yaml_config::ParseUint64(yaml_, name, full_path_, config_vars_ptr_);
}

uint64_t YamlConfig::ParseUint64(const std::string& name,
                                 uint64_t default_value) const {
  return yaml_config::ParseOptionalUint64(yaml_, name, full_path_,
                                          config_vars_ptr_)
      .value_or(default_value);
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
                                    const std::string& default_value) const {
  return yaml_config::ParseOptionalString(yaml_, name, full_path_,
                                          config_vars_ptr_)
      .value_or(default_value);
}

boost::optional<std::string> YamlConfig::ParseOptionalString(
    const std::string& name) const {
  return yaml_config::ParseOptionalString(yaml_, name, full_path_,
                                          config_vars_ptr_);
}

std::chrono::milliseconds YamlConfig::ParseDuration(
    const std::string& name) const {
  return utils::StringToDuration(ParseString(name));
}

std::chrono::milliseconds YamlConfig::ParseDuration(
    const std::string& name, std::chrono::milliseconds default_value) const {
  const auto val = ParseOptionalString(name);
  if (val) {
    return utils::StringToDuration(*val);
  }
  return default_value;
}

boost::optional<std::chrono::milliseconds> YamlConfig::ParseOptionalDuration(
    const std::string& name) const {
  const auto val = ParseOptionalString(name);
  if (val) {
    return utils::StringToDuration(*val);
  }

  return {};
}

}  // namespace yaml_config
