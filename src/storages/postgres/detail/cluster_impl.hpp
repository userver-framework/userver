#pragma once

#include <atomic>
#include <unordered_map>

#include <engine/task/task_processor.hpp>
#include <storages/postgres/cluster_types.hpp>
#include <storages/postgres/options.hpp>
#include <storages/postgres/pool.hpp>
#include <storages/postgres/statistics.hpp>
#include <storages/postgres/transaction.hpp>

#include <storages/postgres/detail/topology.hpp>

namespace storages {
namespace postgres {
namespace detail {

class ClusterImpl {
 public:
  ClusterImpl(const ClusterDescription& cluster_desc,
              engine::TaskProcessor& bg_task_processor, size_t initial_size,
              size_t max_size);
  // TODO pass conninfo and SplitByHost ?
  ClusterImpl(const DSNList& dsn_list, engine::TaskProcessor& bg_task_processor,
              size_t initial_size, size_t max_size);

  ClusterStatistics GetStatistics() const;

  Transaction Begin(ClusterHostType ht, const TransactionOptions& options);

 private:
  void InitPools(const DSNList& dsn_list, size_t initial_size, size_t max_size);

 private:
  ClusterTopologyPtr topology_;
  engine::TaskProcessor& bg_task_processor_;
  // TODO: consider using string_view
  std::unordered_map<std::string, ConnectionPool> host_pools_;
  std::atomic<uint32_t> host_ind_;
};

}  // namespace detail
}  // namespace postgres
}  // namespace storages
