#pragma once

#include <memory>
#include <string>
#include <vector>

#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

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

/// Artificial error injection settings
struct Settings final {
  /// error injection enabled
  bool enabled{false};

  /// error probability from range [0, 1]
  double probability{0};

  /// possible verdicts, will be chosen randomly, Verdict::Error if unspecified
  std::vector<Verdict> possible_verdicts;
};

Settings Parse(const yaml_config::YamlConfig& value,
               formats::parse::To<Settings>);

}  // namespace error_injection

USERVER_NAMESPACE_END
