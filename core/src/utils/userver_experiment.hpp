#pragma once

#include <cstddef>

#include <formats/yaml/value.hpp>

namespace utils {

enum class UserverExperiment {
  kCount,
};

bool IsUserverExperimentEnabled(UserverExperiment);

void EnableUserverExperiment(UserverExperiment);
void DisableUserverExperiment(UserverExperiment);

void ParseUserverExperiments(const formats::yaml::Value& yaml);

}  // namespace utils
