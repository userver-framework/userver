#pragma once

#include <atomic>
#include <memory>
#include <unordered_map>

#include <engine/task/task_processor.hpp>
#include <error_injection/settings.hpp>
#include <utils/periodic_task.hpp>
#include <utils/swappingsmart.hpp>

#include <storages/postgres/cluster_types.hpp>
#include <storages/postgres/detail/non_transaction.hpp>
#include <storages/postgres/detail/quorum_commit.hpp>
#include <storages/postgres/options.hpp>
#include <storages/postgres/pool.hpp>
#include <storages/postgres/statistics.hpp>
#include <storages/postgres/transaction.hpp>

namespace storages {
namespace postgres {
namespace detail {

class ClusterImpl {
 public:
  ClusterImpl(const DSNList& dsns, engine::TaskProcessor& bg_task_processor,
              PoolSettings pool_settings, ConnectionSettings conn_settings,
              CommandControl default_cmd_ctl,
              const error_injection::Settings& ei_settings);
  ~ClusterImpl();

  ClusterStatisticsPtr GetStatistics() const;

  Transaction Begin(ClusterHostType ht, const TransactionOptions& options,
                    engine::Deadline deadline, OptionalCommandControl = {});

  NonTransaction Start(ClusterHostType host_type, engine::Deadline deadline);

  void SetDefaultCommandControl(CommandControl);
  SharedCommandControl GetDefaultCommandControl() const {
    return default_cmd_ctl_.Get();
  }

 private:
  using ConnectionPoolPtr = std::shared_ptr<ConnectionPool>;
  using HostPoolByDsn = std::unordered_map<std::string, ConnectionPoolPtr>;

  ClusterImpl(engine::TaskProcessor& bg_task_processor,
              PoolSettings pool_settings, ConnectionSettings conn_settings,
              CommandControl default_cmd_ctl,
              const error_injection::Settings& ei_settings);

  void InitPools(const DSNList& dsn_list);
  ConnectionPoolPtr GetPool(const std::string& dsn) const;
  ConnectionPoolPtr FindPool(ClusterHostType ht);

 private:
  QuorumCommitTopologyPtr topology_;
  engine::TaskProcessor& bg_task_processor_;
  ::utils::PeriodicTask periodic_task_;
  // This variable should never be used directly as it may be modified
  // concurrently.
  // Obtain needed pool with GetPool call.
  // Don't try to modify the variable from two different places as it may result
  // in lost updates.
  // Places of direct usage:
  // - InitPools - pool initialization (before use)
  // - GetPool - get needed pool with atomicity guarantees
  // - CheckTopology - single place of modification
  ::utils::SwappingSmart<const HostPoolByDsn> host_pools_;
  std::atomic<uint32_t> host_ind_;
  PoolSettings pool_settings_;
  ConnectionSettings conn_settings_;
  ::utils::SwappingSmart<const CommandControl> default_cmd_ctl_;
  const error_injection::Settings ei_settings_;
  std::atomic_flag update_lock_;
};

}  // namespace detail
}  // namespace postgres
}  // namespace storages
