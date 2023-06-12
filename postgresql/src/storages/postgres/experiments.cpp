#include <storages/postgres/experiments.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

USERVER_NAMESPACE::utils::impl::UserverExperiment kPipelineExperiment(
    "pg-connection-pipeline-enabled", true);

USERVER_NAMESPACE::utils::impl::UserverExperiment
    kDeadlinePropagationExperiment("pg-deadline-propagation", true);

}  // namespace storages::postgres

USERVER_NAMESPACE_END
