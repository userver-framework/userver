#include <yaml_config/yaml_config.hpp>

#include <utils/string_to_duration.hpp>

namespace yaml_config {

YamlConfig::YamlConfig(formats::yaml::Value yaml, std::string full_path,
                       VariableMapPtr config_vars_ptr)
    : yaml_(std::move(yaml)),
      full_path_(std::move(full_path)),
      config_vars_ptr_(std::move(config_vars_ptr)) {}

YamlConfig YamlConfig::ParseFromYaml(
    const formats::yaml::Value& yaml, const std::string& full_path,
    const yaml_config::VariableMapPtr& config_vars_ptr) {
  return YamlConfig(yaml, full_path, config_vars_ptr);
}

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

std::optional<int> YamlConfig::ParseOptionalInt(const std::string& name) const {
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

std::optional<bool> YamlConfig::ParseOptionalBool(
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

std::optional<uint64_t> YamlConfig::ParseOptionalUint64(
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

std::optional<std::string> YamlConfig::ParseOptionalString(
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

std::optional<std::chrono::milliseconds> YamlConfig::ParseOptionalDuration(
    const std::string& name) const {
  const auto val = ParseOptionalString(name);
  if (val) {
    return utils::StringToDuration(*val);
  }

  return {};
}

YamlConfig YamlConfig::operator[](const std::string& key) const {
  if (!IsMissing()) {
    yaml_.CheckObject();
  }
  auto result_opt = yaml_config::ParseOptional<YamlConfig>(
      yaml_, key, full_path_, config_vars_ptr_);
  if (result_opt)
    return std::move(*result_opt);
  else
    return YamlConfig(formats::yaml::Value()[impl::PathAppend(full_path_, key)],
                      impl::PathAppend(full_path_, key), config_vars_ptr_);
}

YamlConfig YamlConfig::operator[](size_t index) const {
  if (!IsMissing()) {
    yaml_.CheckArray();
  }
  auto result_opt = yaml_config::ParseOptional<YamlConfig>(
      yaml_, index, full_path_, config_vars_ptr_);
  if (result_opt)
    return std::move(*result_opt);
  else
    return YamlConfig(
        formats::yaml::Value()[impl::PathAppend(full_path_, index)],
        impl::PathAppend(full_path_, index), config_vars_ptr_);
}

bool YamlConfig::IsMissing() const { return yaml_.IsMissing(); }

bool YamlConfig::IsNull() const { return yaml_.IsNull(); }

YamlConfig::const_iterator YamlConfig::begin() const {
  return const_iterator{*this, yaml_.begin()};
}

YamlConfig::const_iterator YamlConfig::cbegin() const {
  return const_iterator{*this, yaml_.begin()};
}

YamlConfig::const_iterator YamlConfig::end() const {
  return const_iterator{*this, yaml_.end()};
}

YamlConfig::const_iterator YamlConfig::cend() const {
  return const_iterator{*this, yaml_.end()};
}

}  // namespace yaml_config
