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
  TimeoutDuration timeout = cmd_ctl.is_initialized()
                                ? cmd_ctl->network
                                : GetDefaultCommandControl()->network;
  auto deadline = engine::Deadline::FromDuration(timeout);
  return pimpl_->Begin(ht, options, deadline, cmd_ctl);
}

detail::NonTransaction Cluster::Start(ClusterHostType ht,
                                      engine::Deadline deadline) {
  return pimpl_->Start(ht, deadline);
}

engine::TaskWithResult<void> Cluster::DiscoverTopology() {
  return pimpl_->DiscoverTopology();
}

void Cluster::SetDefaultCommandControl(CommandControl cmd_ctl) {
  pimpl_->SetDefaultCommandControl(cmd_ctl);
}

SharedCommandControl Cluster::GetDefaultCommandControl() const {
  return pimpl_->GetDefaultCommandControl();
}

}  // namespace postgres
}  // namespace storages
