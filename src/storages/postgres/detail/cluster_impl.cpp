#include <storages/postgres/detail/cluster_impl.hpp>

#include <storages/postgres/dsn.hpp>
#include <storages/postgres/exceptions.hpp>

namespace storages {
namespace postgres {
namespace detail {

ClusterImpl::ClusterImpl(const DSNList& dsn_list,
                         engine::TaskProcessor& bg_task_processor,
                         size_t initial_size, size_t max_size)
    : bg_task_processor_(bg_task_processor) {
  InitPools(dsn_list, initial_size, max_size);
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

void ClusterImpl::InitPools(const DSNList& dsn_list, size_t initial_size,
                            size_t max_size) {
  const ClusterHostType slave_types[] = {ClusterHostType::kSyncSlave,
                                         ClusterHostType::kSlave};
  std::unordered_map<ClusterHostType, DSNList, ClusterHostTypeHash>
      dsn_by_host_type;
  for (auto&& dsn : dsn_list) {
    const auto host_list = SplitByHost(dsn);
    // Assume the first DSN is master
    if (!host_list.empty()) {
      dsn_by_host_type[ClusterHostType::kMaster].push_back(host_list[0]);
    }
    size_t slave_type_ind = 0;
    for (size_t i = 1; i < host_list.size(); ++i) {
      dsn_by_host_type[slave_types[slave_type_ind]].push_back(host_list[i]);
      slave_type_ind = 1 - slave_type_ind;
    }
  }

  for (auto[host_type, host_dsn] : dsn_by_host_type) {
    host_pools_.emplace(std::piecewise_construct,
                        std::forward_as_tuple(host_type),
                        std::forward_as_tuple(host_dsn, bg_task_processor_,
                                              initial_size, max_size));
    LOG_INFO() << "Added pool of " << host_type << " hosts with "
               << host_dsn.size() << " DSN names";
  }
}

}  // namespace detail
}  // namespace postgres
}  // namespace storages
