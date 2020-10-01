#include <utils/userver_experiment.hpp>

#include <array>
#include <string>
#include <unordered_map>

#include <logging/log.hpp>

namespace utils {
namespace {

auto& GetUserverExperimentFlags() {
  static std::array<bool, static_cast<size_t>(UserverExperiment::kCount)>
      experiments{};
  return experiments;
}

}  // namespace

bool IsUserverExperimentEnabled(UserverExperiment exp) {
  return GetUserverExperimentFlags()[static_cast<size_t>(exp)];
}

void EnableUserverExperiment(UserverExperiment exp) {
  GetUserverExperimentFlags()[static_cast<size_t>(exp)] = true;
}

void DisableUserverExperiment(UserverExperiment exp) {
  GetUserverExperimentFlags()[static_cast<size_t>(exp)] = false;
}

void ParseUserverExperiments(const formats::yaml::Value& yaml) {
  static const std::unordered_map<std::string, UserverExperiment>
      kExperimentByName = {};

  if (yaml.IsMissing()) return;

  for (const auto& exp : yaml) {
    auto it = kExperimentByName.find(exp.As<std::string>());
    if (it != kExperimentByName.end()) {
      LOG_WARNING() << "Enabling userver experiment " << it->first;
      EnableUserverExperiment(it->second);
    }
  }
}

}  // namespace utils
