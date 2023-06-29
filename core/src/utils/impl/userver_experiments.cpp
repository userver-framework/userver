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
    if (value) LOG_INFO() << "Enabled experiment " << experiment.GetName();
    experiment.enabled_ = value;
  }
};

namespace {

using ExperimentPtr = utils::NotNull<UserverExperiment*>;

auto& GetExperimentsInfo() noexcept {
  static std::unordered_map<std::string_view, ExperimentPtr> experiments_info;
  return experiments_info;
}

void RegisterExperiment(UserverExperiment& experiment) {
  utils::impl::AssertStaticRegistrationAllowed("UserverExperiment creation");
  const auto [_, success] =
      GetExperimentsInfo().try_emplace(experiment.GetName(), experiment);
  UINVARIANT(
      success,
      fmt::format("userver experiment with name '{}' is already registered",
                  experiment.GetName()));
}

auto GetEnabledUserverExperiments() {
  return utils::AsContainer<std::vector<ExperimentPtr>>(
      GetExperimentsInfo() | boost::adaptors::map_values |
      boost::adaptors::filtered(
          [](ExperimentPtr ptr) { return ptr->IsEnabled(); }));
}

}  // namespace

UserverExperiment::UserverExperiment(std::string name,
                                     bool force_enabling_allowed)
    : name_(name), force_enabling_allowed_(force_enabling_allowed) {
  RegisterExperiment(*this);
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
    const UserverExperimentSet& enabled_experiments, bool force_enable) {
  utils::impl::AssertStaticRegistrationFinished();

  const auto& exp_map = GetExperimentsInfo();
  for (const auto& name : enabled_experiments) {
    if (exp_map.find(name) == exp_map.end()) {
      throw InvalidUserverExperiments(
          fmt::format("Unknown userver experiment '{}'", name));
    }
  }

  for (const auto& [_, experiment] : exp_map) {
    const bool enabled = enabled_experiments.count(experiment->GetName()) != 0;
    const bool force_enabled =
        experiment->IsForceEnablingAllowed() && force_enable;
    UserverExperimentSetter::Set(*experiment, enabled || force_enabled);
  }
}

UserverExperiment kRedisClusterAutoTopologyExperiment{
    "redis-cluster-autotopology"};

UserverExperiment kPhdrCacheExperiment{"phdr-cache"};

UserverExperiment kGrpcClientDeadlinePropagationExperiment{
    "grpc-client-deadline-propagation"};
UserverExperiment kGrpcServerDeadlinePropagationExperiment{
    "grpc-server-deadline-propagation"};

}  // namespace utils::impl

USERVER_NAMESPACE_END
