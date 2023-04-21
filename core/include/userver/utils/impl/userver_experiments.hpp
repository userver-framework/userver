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
  explicit UserverExperiment(std::string name,
                             bool force_enabled = false) noexcept;

  UserverExperiment(UserverExperiment&&) = delete;
  UserverExperiment& operator=(UserverExperiment&&) = delete;

  bool IsEnabled() const noexcept { return enabled_; }
  bool ForceEnabled() const { return force_enabled_; }
  const std::string& GetName() const { return name_; }

 private:
  friend struct UserverExperimentSetter;

  std::string name_;
  bool enabled_{false};
  bool force_enabled_{false};
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

}  // namespace utils::impl

USERVER_NAMESPACE_END
