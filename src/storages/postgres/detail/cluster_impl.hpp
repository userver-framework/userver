#pragma once

#include <unordered_map>

#include <engine/task/task_processor.hpp>
#include <storages/postgres/cluster_host_type.hpp>
#include <storages/postgres/dsn.hpp>
#include <storages/postgres/options.hpp>
#include <storages/postgres/pool.hpp>
#include <storages/postgres/transaction.hpp>

namespace storages {
namespace postgres {
namespace detail {

struct ClusterHostTypeHash {
  size_t operator()(const ClusterHostType& ht) const noexcept {
    return static_cast<size_t>(ht);
  }
};

class ClusterImpl {
 public:
  ClusterImpl(const DSNList& dsn_list, engine::TaskProcessor& bg_task_processor,
              size_t initial_size, size_t max_size);

  Transaction Begin(ClusterHostType ht, const TransactionOptions& options);

 private:
  void InitPools(const DSNList& dsn_list, size_t initial_size, size_t max_size);

 private:
  engine::TaskProcessor& bg_task_processor_;
  std::unordered_map<ClusterHostType, ConnectionPool, ClusterHostTypeHash>
      host_pools_;
};

}  // namespace detail
}  // namespace postgres
}  // namespace storages
