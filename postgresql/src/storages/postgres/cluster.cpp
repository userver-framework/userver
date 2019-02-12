#include <storages/postgres/cluster.hpp>

#include <storages/postgres/detail/cluster_impl.hpp>

namespace storages {
namespace postgres {

Cluster::Cluster(const ClusterDescription& cluster_desc,
                 engine::TaskProcessor& bg_task_processor,
                 size_t initial_idle_connection_pool_size,
                 size_t max_connection_pool_size, CommandControl cmd_ctl) {
  pimpl_ = std::make_unique<detail::ClusterImpl>(
      cluster_desc, bg_task_processor, initial_idle_connection_pool_size,
      max_connection_pool_size, cmd_ctl);
}

Cluster::~Cluster() = default;

ClusterStatistics Cluster::GetStatistics() const {
  return pimpl_->GetStatistics();
}

Transaction Cluster::Begin(const TransactionOptions& options,
                           OptionalCommandControl cmd_ctl) {
  return Begin(ClusterHostType::kAny, options, cmd_ctl);
}

Transaction Cluster::Begin(ClusterHostType ht,
                           const TransactionOptions& options,
                           OptionalCommandControl cmd_ctl) {
  return pimpl_->Begin(ht, options, cmd_ctl);
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

engine::TaskWithResult<void> Cluster::DiscoverTopology() {
  return pimpl_->DiscoverTopology();
}

void Cluster::SetDefaultCommandControl(CommandControl cmd_ctl) {
  pimpl_->SetDefaultCommandControl(cmd_ctl);
}

}  // namespace postgres
}  // namespace storages
