#pragma once

#include <memory>
#include <string>
#include <vector>

#include <formats/yaml/value_fwd.hpp>
#include <yaml_config/variable_map_fwd.hpp>

namespace error_injection {

/// What error injection hook may decide to do
enum class Verdict {
  Error,        ///< return error
  Timeout,      ///< wait for deadline and return error
  MaxDelay,     ///< wait for deadline w/o returning an error
  RandomDelay,  ///< wait for [0; deadline] w/o retunring an error

  // Skip must be the last

  Skip,  ///< no error
};

struct Settings final {
  bool enabled{false};
  double probability{0};
  std::vector<Verdict> possible_verdicts;

  static Settings ParseFromYaml(
      const formats::yaml::Value& yaml, const std::string& full_path,
      const std::shared_ptr<yaml_config::VariableMap>& config_vars_ptr);
};

}  // namespace error_injection
