#include <storages/postgres/detail/topology/standalone.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail::topology {

Standalone::Standalone(engine::TaskProcessor& bg_task_processor, DsnList dsns,
                       clients::dns::Resolver* resolver,
                       const TopologySettings& topology_settings,
                       const ConnectionSettings& conn_settings,
                       const DefaultCommandControls& default_cmd_ctls,
                       const testsuite::PostgresControl& testsuite_pg_ctl,
                       error_injection::Settings ei_settings)
    : TopologyBase(bg_task_processor, std::move(dsns), resolver,
                   topology_settings, conn_settings, default_cmd_ctls,
                   testsuite_pg_ctl, std::move(ei_settings)),
      dsn_indices_by_type_(DsnIndicesByType{{ClusterHostType::kMaster, {0}}}),
      alive_dsn_indices_(DsnIndices{0}),
      dsn_stats_(GetDsnList().size()) {
  UASSERT(GetDsnList().size() == 1);
}

rcu::ReadablePtr<TopologyBase::DsnIndicesByType>
Standalone::GetDsnIndicesByType() const {
  return dsn_indices_by_type_.Read();
}

rcu::ReadablePtr<TopologyBase::DsnIndices> Standalone::GetAliveDsnIndices()
    const {
  return alive_dsn_indices_.Read();
}

const std::vector<decltype(InstanceStatistics::topology)>&
Standalone::GetDsnStatistics() const {
  return dsn_stats_;
}

}  // namespace storages::postgres::detail::topology

USERVER_NAMESPACE_END
