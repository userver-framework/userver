#include <storages/postgres/cluster.hpp>

#include <storages/postgres/detail/cluster_impl.hpp>
#include <storages/postgres/detail/topology.hpp>

namespace storages {
namespace postgres {

Cluster::Cluster(const ClusterDescription& cluster_desc,
                 engine::TaskProcessor& bg_task_processor,
                 size_t initial_idle_connection_pool_size,
                 size_t max_connection_pool_size) {
  detail::ClusterTopology topology(cluster_desc);
  pimpl_ = std::make_unique<detail::ClusterImpl>(
      std::move(topology), bg_task_processor, initial_idle_connection_pool_size,
      max_connection_pool_size);
}

Cluster::~Cluster() = default;

ClusterStatistics Cluster::GetStatistics() const {
  return pimpl_->GetStatistics();
}

Transaction Cluster::Begin(const TransactionOptions& options) {
  return Begin(ClusterHostType::kAny, options);
}

Transaction Cluster::Begin(ClusterHostType ht,
                           const TransactionOptions& options) {
  return pimpl_->Begin(ht, options);
}

ResultSet Cluster::Execute(const TransactionOptions& options,
                           const std::string& statement) {
  return Execute(ClusterHostType::kAny, options, statement);
}

ResultSet Cluster::Execute(ClusterHostType ht,
                           const TransactionOptions& options,
                           const std::string& statement) {
  return Execute<>(ht, options, statement);
}

}  // namespace postgres
}  // namespace storages
