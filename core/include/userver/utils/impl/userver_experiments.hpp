#pragma once

#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include <userver/utils/not_null.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

class UserverExperiment final {
 public:
  // Setting 'userver_experiments_force_enabled: true' in the static config
  // root results in batch-enabling the experiments created with
  // 'force_enabling_allowed = true'.
  explicit UserverExperiment(std::string name,
                             bool force_enabling_allowed = false);

  UserverExperiment(UserverExperiment&&) = delete;
  UserverExperiment& operator=(UserverExperiment&&) = delete;

  bool IsEnabled() const noexcept { return enabled_; }
  bool IsForceEnablingAllowed() const { return force_enabling_allowed_; }
  const std::string& GetName() const { return name_; }

 private:
  friend struct UserverExperimentSetter;

  std::string name_;
  bool enabled_{false};
  bool force_enabling_allowed_{false};
};

class InvalidUserverExperiments final : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

using UserverExperimentSet = std::unordered_set<std::string>;

/// Reverts all changes to experiments in the destructor
class UserverExperimentsScope final {
 public:
  UserverExperimentsScope();

  UserverExperimentsScope(UserverExperimentsScope&&) = delete;
  UserverExperimentsScope& operator=(UserverExperimentsScope&&) = delete;
  ~UserverExperimentsScope();

  void Set(UserverExperiment& experiment, bool value) noexcept;

  /// @throws InvalidUserverExperiments on name mismatch
  void EnableOnly(const UserverExperimentSet& enabled_experiments,
                  bool force_enable = false);

 private:
  const std::vector<utils::NotNull<UserverExperiment*>> old_enabled_;
};

// TODO move to userver/mongo once the issues with linker are resolved.
extern UserverExperiment kRedisClusterAutoTopologyExperiment;

extern UserverExperiment kPhdrCacheExperiment;

// TODO move to userver/grpc once the issues with linker are resolved.
extern UserverExperiment kGrpcClientDeadlinePropagationExperiment;
extern UserverExperiment kGrpcServerDeadlinePropagationExperiment;

}  // namespace utils::impl

USERVER_NAMESPACE_END
