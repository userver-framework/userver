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
    explicit UserverExperiment(std::string name);

    UserverExperiment(UserverExperiment&&) = delete;
    UserverExperiment& operator=(UserverExperiment&&) = delete;

    bool IsEnabled() const noexcept { return enabled_; }
    const std::string& GetName() const { return name_; }

private:
    friend struct UserverExperimentSetter;

    std::string name_;
    bool enabled_{false};
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
    void EnableOnly(const UserverExperimentSet& enabled_experiments);

private:
    const std::vector<utils::NotNull<UserverExperiment*>> old_enabled_;
};

// All userver experiments should be registered here to ensure that they can be globally enabled
// for all services, no matter whether a specific userver library is used in a given service.
extern UserverExperiment kJemallocBgThread;
extern UserverExperiment kServerSelectionTimeoutExperiment;
extern UserverExperiment kPgCcExperiment;
extern UserverExperiment kYdbDeadlinePropagationExperiment;

}  // namespace utils::impl

USERVER_NAMESPACE_END
