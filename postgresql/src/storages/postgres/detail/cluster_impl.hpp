#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <engine/task/task_processor.hpp>
#include <error_injection/settings.hpp>
#include <utils/swappingsmart.hpp>

#include <storages/postgres/cluster_types.hpp>
#include <storages/postgres/detail/non_transaction.hpp>
#include <storages/postgres/detail/quorum_commit.hpp>
#include <storages/postgres/options.hpp>
#include <storages/postgres/pool.hpp>
#include <storages/postgres/statistics.hpp>
#include <storages/postgres/transaction.hpp>
#include <testsuite/postgres_control.hpp>

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

  Transaction Begin(ClusterHostType ht, const TransactionOptions& options,
                    OptionalCommandControl = {});

  NonTransaction Start(ClusterHostType host_type);

  void SetDefaultCommandControl(CommandControl);
  SharedCommandControl GetDefaultCommandControl() const {
    return default_cmd_ctl_.Get();
  }

 private:
  using ConnectionPoolPtr = std::shared_ptr<ConnectionPool>;

  ConnectionPoolPtr FindPool(ClusterHostType ht);

 private:
  QuorumCommitTopology topology_;
  engine::TaskProcessor& bg_task_processor_;
  std::vector<ConnectionPoolPtr> host_pools_;
  std::atomic<uint32_t> rr_host_idx_;
  ::utils::SwappingSmart<const CommandControl> default_cmd_ctl_;
  testsuite::PostgresControl testsuite_pg_ctl_;
  std::atomic_flag update_lock_;
};

}  // namespace storages::postgres::detail
