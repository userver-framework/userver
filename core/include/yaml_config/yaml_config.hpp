#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

#include <formats/yaml.hpp>
#include <yaml_config/value.hpp>

#include "variable_map.hpp"

namespace yaml_config {

class YamlConfig {
 public:
  YamlConfig(formats::yaml::Value yaml, std::string full_path,
             VariableMapPtr config_vars_ptr);

  const formats::yaml::Value& Yaml() const;
  const std::string& FullPath() const;
  const VariableMapPtr& ConfigVarsPtr() const;

  int ParseInt(const std::string& name) const;
  int ParseInt(const std::string& name, int default_value) const;
  std::optional<int> ParseOptionalInt(const std::string& name) const;
  bool ParseBool(const std::string& name) const;
  bool ParseBool(const std::string& name, bool default_value) const;
  std::optional<bool> ParseOptionalBool(const std::string& name) const;
  uint64_t ParseUint64(const std::string& name) const;
  uint64_t ParseUint64(const std::string& name, uint64_t default_value) const;
  std::optional<uint64_t> ParseOptionalUint64(const std::string& name) const;
  std::string ParseString(const std::string& name) const;
  std::string ParseString(const std::string& name,
                          const std::string& default_value) const;
  std::optional<std::string> ParseOptionalString(const std::string& name) const;

  std::chrono::milliseconds ParseDuration(const std::string& name) const;
  std::chrono::milliseconds ParseDuration(
      const std::string& name, std::chrono::milliseconds default_value) const;
  std::optional<std::chrono::milliseconds> ParseOptionalDuration(
      const std::string& name) const;

  template <typename T>
  T Parse(const std::string& name) const {
    return yaml_config::Parse<T>(yaml_, name, full_path_, config_vars_ptr_);
  }

  template <typename T>
  T Parse(const std::string& name, const T& default_value) const {
    return yaml_config::Parse<std::optional<T>>(yaml_, name, full_path_,
                                                config_vars_ptr_)
        .value_or(default_value);
  }

  template <typename T>
  std::decay_t<T> Parse(const std::string& name, T&& default_value) const {
    return yaml_config::Parse<std::optional<std::decay_t<T>>>(
               yaml_, name, full_path_, config_vars_ptr_)
        .value_or(std::forward<T>(default_value));
  }

 private:
  formats::yaml::Value yaml_;
  std::string full_path_;
  VariableMapPtr config_vars_ptr_;
};

}  // namespace yaml_config
