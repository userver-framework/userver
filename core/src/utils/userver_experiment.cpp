#include <utils/userver_experiment.hpp>

#include <array>
#include <string>

#include <userver/logging/log.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

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
  if (yaml.IsMissing()) return;

  static constexpr utils::TrivialBiMap kExperiments = [](auto selector) {
    return selector().Case("jemalloc-bg-thread",
                           UserverExperiment::kJemallocBgThread);
  };

  for (const auto& exp : yaml) {
    auto value = exp.As<std::string>();
    auto exp_enum = kExperiments.TryFind(value);
    if (exp_enum) {
      LOG_WARNING() << "Enabling userver experiment " << value;
      EnableUserverExperiment(*exp_enum);
    }
  }
}

}  // namespace utils

USERVER_NAMESPACE_END
