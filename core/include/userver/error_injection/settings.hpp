#pragma once

#include <memory>
#include <string>
#include <vector>

#include <yaml_config/yaml_config.hpp>

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
};

Settings Parse(const yaml_config::YamlConfig& value,
               formats::parse::To<Settings>);

}  // namespace error_injection
