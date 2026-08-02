#include <storages/odbc/detail/topology/standalone.hpp>

#include <storages/odbc/detail/pool.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail::topology {

Standalone::Standalone(
    const settings::ODBCClusterSettings& settings,
    const settings::StatementMetricsSettings& statement_metrics_settings,
    clients::dns::Resolver* resolver,
    engine::TaskProcessor& blocking_task_processor
)
    : TopologyBase(settings, statement_metrics_settings, resolver, blocking_task_processor),
      pool_{InitializePoolReference()}
{}

Standalone::~Standalone() = default;

Pool& Standalone::GetPrimary() const { return pool_; }
Pool& Standalone::GetSecondary() const { return pool_; }

Pool& Standalone::InitializePoolReference() const {
    UASSERT(GetPools().size() == 1);
    return *GetPools().front();
}

}  // namespace storages::odbc::detail::topology

USERVER_NAMESPACE_END
