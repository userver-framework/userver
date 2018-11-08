#include <storages/postgres/detail/cluster_impl.hpp>

#include <storages/postgres/dsn.hpp>
#include <storages/postgres/exceptions.hpp>

namespace storages {
namespace postgres {
namespace detail {

ClusterImpl::ClusterImpl(ClusterTopology&& topology,
                         engine::TaskProcessor& bg_task_processor,
                         size_t initial_size, size_t max_size)
    : topology_(std::move(topology)),
      bg_task_processor_(bg_task_processor),
      host_ind_(0) {
  const auto& dsn_list = topology_.GetDsnList();
  host_pools_.reserve(dsn_list.size());
  for (auto&& dsn : dsn_list) {
    host_pools_.emplace_back(dsn, bg_task_processor_, initial_size, max_size);
  }
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

  const auto& host_indices = topology_.GetHostsByType(host_type);
  if (host_indices.empty()) {
    throw ClusterUnavailable("Pool for host type (passed: " + ToString(ht) +
                             ", picked: " + ToString(host_type) +
                             ") is not available");
  }

  LOG_DEBUG() << "Starting transaction on the host of " << host_type << " type";
  const auto ind =
      host_indices[host_ind_.fetch_add(1, std::memory_order_relaxed) %
                   host_indices.size()];
  return host_pools_[ind].Begin(options);
}

}  // namespace detail
}  // namespace postgres
}  // namespace storages
