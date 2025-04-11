#include "deadline.hpp"

#include <storages/postgres/experiments.hpp>
#include <storages/postgres/postgres_config.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/utils/impl/userver_experiments.hpp>

#include <dynamic_config/variables/POSTGRES_DEADLINE_PROPAGATION_VERSION.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

void CheckDeadlineIsExpired(const dynamic_config::Snapshot& config) {
    if (config[::dynamic_config::POSTGRES_DEADLINE_PROPAGATION_VERSION] != kDeadlinePropagationExperimentVersion) {
        return;
    }

    const auto inherited_deadline = server::request::GetTaskInheritedDeadline();
    if (inherited_deadline.IsReached()) {
        server::request::MarkTaskInheritedDeadlineExpired();
        throw ConnectionInterrupted("Cancelled by deadline");
    }
}

TimeoutDuration AdjustTimeout(TimeoutDuration timeout, bool& adjusted) {
    adjusted = false;

    const auto inherited_deadline = server::request::GetTaskInheritedDeadline();
    if (!inherited_deadline.IsReachable()) return timeout;

    auto left = std::chrono::duration_cast<TimeoutDuration>(inherited_deadline.TimeLeft());
    if (left.count() < 0) {
        server::request::MarkTaskInheritedDeadlineExpired();
        throw ConnectionInterrupted("Cancelled by deadline");
    }

    if (timeout > left) {
        adjusted = true;
        return left;
    } else {
        return timeout;
    }
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
