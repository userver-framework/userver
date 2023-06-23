#include "deadline.hpp"

#include <storages/postgres/experiments.hpp>
#include <storages/postgres/postgres_config.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/storages/postgres/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

void CheckDeadlineIsExpired(const dynamic_config::Snapshot& config) {
  if (!kDeadlinePropagationExperiment.IsEnabled() ||
      config[kDeadlinePropagationVersionConfig] !=
          kDeadlinePropagationExperimentVersion) {
    return;
  }

  const auto inherited_deadline = server::request::GetTaskInheritedDeadline();
  if (inherited_deadline.IsReached()) {
    throw ConnectionInterrupted("Cancelled by deadline");
  }
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
