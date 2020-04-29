#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <engine/task/task_processor.hpp>
#include <error_injection/settings.hpp>
#include <testsuite/postgres_control.hpp>

#include <storages/postgres/cluster_types.hpp>
#include <storages/postgres/detail/non_transaction.hpp>
#include <storages/postgres/options.hpp>
#include <storages/postgres/statistics.hpp>
#include <storages/postgres/transaction.hpp>

#include <storages/postgres/detail/pg_impl_types.hpp>
#include <storages/postgres/detail/pool.hpp>
#include <storages/postgres/detail/quorum_commit.hpp>

namespace storages::postgres::detail {

class ClusterImpl {
 public:
  ClusterImpl(DsnList dsns, engine::TaskProcessor& bg_task_processor,
              const PoolSettings& pool_settings,
              const ConnectionSettings& conn_settings,
              const CommandControl& default_cmd_ctl,
              const testsuite::PostgresControl& testsuite_pg_ctl,
              const error_injection::Settings& ei_settings);
  ~ClusterImpl();

  ClusterStatisticsPtr GetStatistics() const;

  Transaction Begin(ClusterHostTypeFlags, const TransactionOptions&,
                    OptionalCommandControl);

  NonTransaction Start(ClusterHostTypeFlags, OptionalCommandControl);

  void SetDefaultCommandControl(CommandControl, DefaultCommandControlSource);
  CommandControl GetDefaultCommandControl() const;

 private:
  using ConnectionPoolPtr = std::shared_ptr<ConnectionPool>;

  ConnectionPoolPtr FindPool(ClusterHostTypeFlags);

 private:
  QuorumCommitTopology topology_;
  engine::TaskProcessor& bg_task_processor_;
  std::vector<ConnectionPoolPtr> host_pools_;
  std::atomic<uint32_t> rr_host_idx_;
};

}  // namespace storages::postgres::detail
