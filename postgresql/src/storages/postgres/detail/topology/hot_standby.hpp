#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <storages/postgres/detail/topology/base.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/storages/postgres/statistics.hpp>
#include <userver/utils/periodic_task.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail::topology {

/// Default topology scanner, suitable for quorum commit clusters
class HotStandby final : public TopologyBase {
 public:
  HotStandby(engine::TaskProcessor& bg_task_processor, DsnList dsns,
             clients::dns::Resolver* resolver,
             const TopologySettings& topology_settings,
             const ConnectionSettings& conn_settings,
             const DefaultCommandControls& default_cmd_ctls,
             const testsuite::PostgresControl& testsuite_pg_ctl,
             error_injection::Settings ei_settings);

  ~HotStandby() override;

  rcu::ReadablePtr<DsnIndicesByType> GetDsnIndicesByType() const override;
  rcu::ReadablePtr<DsnIndices> GetAliveDsnIndices() const override;
  const std::vector<decltype(InstanceStatistics::topology)>& GetDsnStatistics()
      const override;

 private:
  struct HostState;

  void RunDiscovery();
  void RunCheck(DsnIndex);

  std::vector<HostState> host_states_;
  rcu::Variable<DsnIndicesByType> dsn_indices_by_type_;
  rcu::Variable<DsnIndices> alive_dsn_indices_;
  std::vector<decltype(InstanceStatistics::topology)> dsn_stats_;
  USERVER_NAMESPACE::utils::PeriodicTask discovery_task_;
};

/// Returns sync slave names (disregarding availability)
std::vector<std::string> ParseSyncStandbyNames(std::string_view value);

}  // namespace storages::postgres::detail::topology

USERVER_NAMESPACE_END
