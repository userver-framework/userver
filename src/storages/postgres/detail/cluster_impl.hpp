#pragma once

#include <atomic>
#include <vector>

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
  ClusterImpl(ClusterTopology&& topology,
              engine::TaskProcessor& bg_task_processor, size_t initial_size,
              size_t max_size);

  ClusterStatistics GetStatistics() const;

  Transaction Begin(ClusterHostType ht, const TransactionOptions& options);

 private:
  ClusterTopology topology_;
  engine::TaskProcessor& bg_task_processor_;
  std::vector<ConnectionPool> host_pools_;
  std::atomic<uint32_t> host_ind_;
};

}  // namespace detail
}  // namespace postgres
}  // namespace storages
