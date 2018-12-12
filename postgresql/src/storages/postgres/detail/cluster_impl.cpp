#include <storages/postgres/detail/cluster_impl.hpp>

#include <engine/async.hpp>

#include <storages/postgres/detail/topology_discovery.hpp>
#include <storages/postgres/detail/topology_proxy.hpp>
#include <storages/postgres/dsn.hpp>
#include <storages/postgres/exceptions.hpp>

namespace storages {
namespace postgres {
namespace detail {

ClusterImpl::ClusterImpl(const ClusterDescription& cluster_desc,
                         engine::TaskProcessor& bg_task_processor,
                         size_t initial_size, size_t max_size)
    : bg_task_processor_(bg_task_processor), host_ind_(0) {
  topology_ = std::make_unique<detail::ClusterTopologyProxy>(cluster_desc);
  const auto& dsn_list = topology_->GetDsnList();
  InitPools(dsn_list, initial_size, max_size);
}

ClusterImpl::ClusterImpl(const DSNList& dsn_list,
                         engine::TaskProcessor& bg_task_processor,
                         size_t initial_size, size_t max_size)
    : bg_task_processor_(bg_task_processor), host_ind_(0) {
  topology_ = std::make_unique<detail::ClusterTopologyDiscovery>(
      bg_task_processor_, dsn_list);
  InitPools(dsn_list, initial_size, max_size);
}

void ClusterImpl::InitPools(const DSNList& dsn_list, size_t initial_size,
                            size_t max_size) {
  std::vector<engine::TaskWithResult<std::pair<std::string, ConnectionPool>>>
      tasks;
  tasks.reserve(dsn_list.size());
  host_pools_.reserve(dsn_list.size());

  for (auto dsn : dsn_list) {
    auto task =
        engine::Async([ this, dsn = std::move(dsn), initial_size, max_size ] {
          return std::make_pair(dsn, ConnectionPool{dsn, bg_task_processor_,
                                                    initial_size, max_size});
        });
    tasks.push_back(std::move(task));
  }

  for (auto&& task : tasks) {
    host_pools_.insert(task.Get());
  }
}

ClusterStatistics ClusterImpl::GetStatistics() const {
  ClusterStatistics cluster_stats;
  auto hosts_by_type = topology_->GetHostsByType();

  // TODO remove code duplication
  const auto& master_dsns = hosts_by_type[ClusterHostType::kMaster];
  if (!master_dsns.empty()) {
    auto dsn = master_dsns[0];
    cluster_stats.master.dsn = dsn;
    auto it_find = host_pools_.find(dsn);
    if (it_find != host_pools_.end()) {
      cluster_stats.master.stats = it_find->second.GetStatistics();
    }
  }

  const auto& sync_slave_dsns = hosts_by_type[ClusterHostType::kSyncSlave];
  if (!sync_slave_dsns.empty()) {
    auto dsn = sync_slave_dsns[0];
    cluster_stats.sync_slave.dsn = dsn;
    auto it_find = host_pools_.find(dsn);
    if (it_find != host_pools_.end()) {
      cluster_stats.sync_slave.stats = it_find->second.GetStatistics();
    }
  }

  const auto& slaves_dsns = hosts_by_type[ClusterHostType::kSlave];
  if (!slaves_dsns.empty()) {
    cluster_stats.slaves.reserve(slaves_dsns.size());
    for (auto&& dsn : slaves_dsns) {
      InstanceStatsDescriptor slave_desc;
      slave_desc.dsn = dsn;
      auto it_find = host_pools_.find(dsn);
      if (it_find != host_pools_.end()) {
        slave_desc.stats = it_find->second.GetStatistics();
      }
      cluster_stats.slaves.push_back(std::move(slave_desc));
    }
  }
  return cluster_stats;
}

Transaction ClusterImpl::Begin(ClusterHostType ht,
                               const TransactionOptions& options) {
  LOG_TRACE() << "Requested transaction on the host of " << ht << " type";
  auto host_type = ht;
  if (options.IsReadOnly()) {
    if (host_type == ClusterHostType::kAny) {
      host_type = ClusterHostType::kSyncSlave;
    }
  } else {
    if (host_type == ClusterHostType::kAny) {
      host_type = ClusterHostType::kMaster;
    } else if (host_type != ClusterHostType::kMaster) {
      throw ClusterUnavailable("Cannot start RW-transaction on a slave");
    }
  }

  auto hosts_by_type = topology_->GetHostsByType();
  const auto& host_dsns = hosts_by_type[host_type];
  if (host_dsns.empty()) {
    throw ClusterUnavailable("Pool for host type (passed: " + ToString(ht) +
                             ", picked: " + ToString(host_type) +
                             ") is not available");
  }

  const auto& dsn =
      host_dsns[host_ind_.fetch_add(1, std::memory_order_relaxed) %
                host_dsns.size()];
  LOG_TRACE() << "Starting transaction on the host of " << host_type << " type";

  auto it_find = host_pools_.find(dsn);
  if (it_find == host_pools_.end()) {
    throw ClusterUnavailable("Host not found for given DSN: " + dsn);
  }
  // TODO Catch connection problems and force topology update
  return it_find->second.Begin(options);
}

}  // namespace detail
}  // namespace postgres
}  // namespace storages
