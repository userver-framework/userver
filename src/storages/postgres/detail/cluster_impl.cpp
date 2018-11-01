#include <storages/postgres/detail/cluster_impl.hpp>

#include <storages/postgres/dsn.hpp>
#include <storages/postgres/exceptions.hpp>

namespace storages {
namespace postgres {
namespace detail {

ClusterImpl::ClusterImpl(const ClusterDescription& cluster_desc,
                         engine::TaskProcessor& bg_task_processor,
                         size_t initial_size, size_t max_size)
    : bg_task_processor_(bg_task_processor) {
  InitPools(cluster_desc, initial_size, max_size);
}

Transaction ClusterImpl::Begin(ClusterHostType ht,
                               const TransactionOptions& options) {
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

  auto it_find = host_pools_.find(host_type);
  if (it_find == host_pools_.end()) {
    throw ClusterUnavailable("Pool for host type (passed: " + ToString(ht) +
                             ", picked: " + ToString(host_type) +
                             ") is not available");
  }

  LOG_DEBUG() << "Starting transaction on the host of " << host_type << " type";
  return it_find->second.Begin(options);
}

void ClusterImpl::InitPools(const ClusterDescription& cluster_desc,
                            size_t initial_size, size_t max_size) {
  if (cluster_desc.master_dsn_.empty()) {
    throw ClusterUnavailable("Master host is unavailable");
  }
  host_pools_.insert(std::make_pair(
      ClusterHostType::kMaster,
      ConnectionPool(DSNList{cluster_desc.master_dsn_}, bg_task_processor_,
                     initial_size, max_size)));
  LOG_INFO() << "Added " << ClusterHostType::kMaster << " host";

  if (!cluster_desc.sync_slave_dsn_.empty()) {
    host_pools_.insert(std::make_pair(
        ClusterHostType::kSyncSlave,
        ConnectionPool(DSNList{cluster_desc.sync_slave_dsn_},
                       bg_task_processor_, initial_size, max_size)));
    LOG_INFO() << "Added " << ClusterHostType::kSyncSlave << " host";
  } else {
    LOG_INFO() << "No " << ClusterHostType::kSyncSlave << " host added";
  }

  if (!cluster_desc.slave_dsns_.empty()) {
    host_pools_.insert(std::make_pair(
        ClusterHostType::kSlave,
        ConnectionPool(cluster_desc.slave_dsns_, bg_task_processor_,
                       initial_size, max_size)));
    LOG_INFO() << "Added pool of " << ClusterHostType::kSlave << " hosts with "
               << cluster_desc.slave_dsns_.size() << " DSN names";
  } else {
    LOG_INFO() << "No " << ClusterHostType::kSlave << " hosts added";
  }
}

}  // namespace detail
}  // namespace postgres
}  // namespace storages
