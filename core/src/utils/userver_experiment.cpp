#include <utils/userver_experiment.hpp>

#include <array>
#include <string>

#include <userver/logging/log.hpp>
#include <userver/utils/consteval_map.hpp>

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
  static constexpr auto kExperimentByName =
      utils::MakeConsinitMap<std::string_view, UserverExperiment>({
          {"jemalloc-bg-thread", UserverExperiment::kJemallocBgThread},
      });

  if (yaml.IsMissing()) return;

  for (const auto& exp : yaml) {
    auto exp_name = exp.As<std::string>();
    auto ptr = kExperimentByName.FindOrNullptr(exp_name);
    if (ptr) {
      LOG_WARNING() << "Enabling userver experiment " << exp_name;
      EnableUserverExperiment(*ptr);
    }
  }
}

}  // namespace utils

USERVER_NAMESPACE_END
