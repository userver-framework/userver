#pragma once

#include <vector>

#include <rcu/rcu.hpp>
#include <storages/postgres/detail/topology/base.hpp>
#include <storages/postgres/statistics.hpp>

namespace storages::postgres::detail::topology {

/// No-op scanner, always reports the single host as an alive master (no RTT)
class Standalone final : public TopologyBase {
 public:
  Standalone(engine::TaskProcessor& bg_task_processor, DsnList dsns,
             const TopologySettings& topology_settings,
             const ConnectionSettings& conn_settings,
             const DefaultCommandControls& default_cmd_ctls,
             const testsuite::PostgresControl& testsuite_pg_ctl,
             error_injection::Settings ei_settings);

  rcu::ReadablePtr<DsnIndicesByType> GetDsnIndicesByType() const override;
  rcu::ReadablePtr<DsnIndices> GetAliveDsnIndices() const override;
  const std::vector<decltype(InstanceStatistics::topology)>& GetDsnStatistics()
      const override;

 private:
  const rcu::Variable<DsnIndicesByType> dsn_indices_by_type_;
  const rcu::Variable<DsnIndices> alive_dsn_indices_;
  const std::vector<decltype(InstanceStatistics::topology)> dsn_stats_;
};

}  // namespace storages::postgres::detail::topology
