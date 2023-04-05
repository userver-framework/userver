#include <userver/utils/impl/userver_experiments.hpp>

#include <unordered_map>

#include <fmt/format.h>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/map.hpp>

#include <userver/logging/log.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/static_registration.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

struct UserverExperimentSetter final {
  static void Set(UserverExperiment& experiment, bool value) noexcept {
    experiment.enabled_ = value;
  }
};

namespace {

using ExperimentPtr = utils::NotNull<UserverExperiment*>;

auto& GetExperimentsInfo() noexcept {
  static std::unordered_map<std::string, ExperimentPtr> experiments_info;
  return experiments_info;
}

void RegisterExperiment(std::string&& name, UserverExperiment& experiment) {
  utils::impl::AssertStaticRegistrationAllowed("UserverExperiment creation");
  const auto [_, success] =
      GetExperimentsInfo().try_emplace(std::move(name), experiment);
  UINVARIANT(
      success,
      fmt::format("userver experiment with name '{}' is already registered",
                  name));
}

auto GetEnabledUserverExperiments() {
  return utils::AsContainer<std::vector<ExperimentPtr>>(
      GetExperimentsInfo() | boost::adaptors::map_values |
      boost::adaptors::filtered(
          [](ExperimentPtr ptr) { return ptr->IsEnabled(); }));
}

}  // namespace

UserverExperiment::UserverExperiment(std::string name) noexcept {
  RegisterExperiment(std::move(name), *this);
}

UserverExperimentsScope::UserverExperimentsScope()
    : old_enabled_(GetEnabledUserverExperiments()) {}

UserverExperimentsScope::~UserverExperimentsScope() {
  for (const auto& [_, experiment] : GetExperimentsInfo()) {
    UserverExperimentSetter::Set(*experiment, false);
  }
  for (const auto& experiment : old_enabled_) {
    UserverExperimentSetter::Set(*experiment, true);
  }
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void UserverExperimentsScope::Set(UserverExperiment& experiment,
                                  bool value) noexcept {
  utils::impl::AssertStaticRegistrationFinished();
  UserverExperimentSetter::Set(experiment, value);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void UserverExperimentsScope::EnableOnly(
    const UserverExperimentSet& enabled_experiments) {
  utils::impl::AssertStaticRegistrationFinished();

  const auto& exp_map = GetExperimentsInfo();
  for (const auto& name : enabled_experiments) {
    if (exp_map.find(name) == exp_map.end()) {
      throw InvalidUserverExperiments(
          fmt::format("Unknown userver experiment '{}'", name));
    }
  }

  for (const auto& [name, experiment] : exp_map) {
    UserverExperimentSetter::Set(*experiment,
                                 enabled_experiments.count(name) != 0);
  }

  if (!enabled_experiments.empty()) {
    LOG_WARNING() << "Enabled userver experiments: " << enabled_experiments;
  }
}

}  // namespace utils::impl

USERVER_NAMESPACE_END
