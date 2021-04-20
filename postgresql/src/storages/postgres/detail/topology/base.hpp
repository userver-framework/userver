#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include <engine/task/task_processor_fwd.hpp>
#include <error_injection/settings.hpp>
#include <storages/postgres/cluster_types.hpp>
#include <storages/postgres/default_command_controls.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/dsn.hpp>
#include <storages/postgres/options.hpp>
#include <storages/postgres/statistics.hpp>
#include <testsuite/postgres_control.hpp>

namespace storages::postgres::detail::topology {

class TopologyBase {
 public:
  using DsnIndex = size_t;
  using DsnIndices = std::vector<DsnIndex>;
  using DsnIndicesByType =
      std::unordered_map<ClusterHostType, DsnIndices, ClusterHostTypeHash>;

  TopologyBase(engine::TaskProcessor& bg_task_processor, DsnList dsns,
               const TopologySettings& topology_settings,
               const ConnectionSettings& conn_settings,
               const DefaultCommandControls& default_cmd_ctls,
               const testsuite::PostgresControl& testsuite_pg_ctl,
               error_injection::Settings ei_settings);

  virtual ~TopologyBase() = default;

  const DsnList& GetDsnList() const;
  const TopologySettings& GetTopologySettings() const;
  const testsuite::PostgresControl& GetTestsuiteControl() const;

  /// Currently determined host types, ordered by rtt
  virtual rcu::ReadablePtr<DsnIndicesByType> GetDsnIndicesByType() const = 0;

  /// Currently accessible hosts
  virtual rcu::ReadablePtr<DsnIndices> GetAliveDsnIndices() const = 0;

  // Returns statistics for each DSN in DsnList
  virtual const std::vector<decltype(InstanceStatistics::topology)>&
  GetDsnStatistics() const = 0;

 protected:
  std::unique_ptr<Connection> MakeTopologyConnection(DsnIndex);

 private:
  engine::TaskProcessor& bg_task_processor_;
  const DsnList dsns_;
  const TopologySettings topology_settings_;
  const ConnectionSettings conn_settings_;
  const DefaultCommandControls default_cmd_ctls_;
  const testsuite::PostgresControl testsuite_pg_ctl_;
  const error_injection::Settings ei_settings_;
};

}  // namespace storages::postgres::detail::topology
