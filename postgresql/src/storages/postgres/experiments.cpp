#include <storages/postgres/experiments.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

USERVER_NAMESPACE::utils::impl::UserverExperiment kPipelineExperiment(
    "pg-connection-pipeline-enabled");

}  // namespace storages::postgres

USERVER_NAMESPACE_END
