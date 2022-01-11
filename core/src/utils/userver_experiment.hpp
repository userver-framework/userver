#pragma once

#include <cstddef>

#include <userver/formats/yaml/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

enum class UserverExperiment {
  kJemallocBgThread,

  kCount,
};

bool IsUserverExperimentEnabled(UserverExperiment);

void EnableUserverExperiment(UserverExperiment);
void DisableUserverExperiment(UserverExperiment);

void ParseUserverExperiments(const formats::yaml::Value& yaml);

}  // namespace utils

USERVER_NAMESPACE_END
