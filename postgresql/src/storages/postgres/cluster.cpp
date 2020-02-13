#include <storages/postgres/cluster.hpp>

#include <storages/postgres/detail/cluster_impl.hpp>

namespace storages {
namespace postgres {

Cluster::Cluster(const DSNList& dsns, engine::TaskProcessor& bg_task_processor,
                 PoolSettings pool_settings, ConnectionSettings conn_settings,
                 CommandControl cmd_ctl,
                 const testsuite::PostgresControl& testsuite_pg_ctl,
                 const error_injection::Settings& ei_settings) {
  pimpl_ = std::make_unique<detail::ClusterImpl>(
      dsns, bg_task_processor, pool_settings, conn_settings, cmd_ctl,
      testsuite_pg_ctl, ei_settings);
}

Cluster::~Cluster() = default;

ClusterStatisticsPtr Cluster::GetStatistics() const {
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

detail::NonTransaction Cluster::Start(ClusterHostType ht) {
  return pimpl_->Start(ht);
}

void Cluster::SetDefaultCommandControl(CommandControl cmd_ctl) {
  pimpl_->SetDefaultCommandControl(cmd_ctl);
}

SharedCommandControl Cluster::GetDefaultCommandControl() const {
  return pimpl_->GetDefaultCommandControl();
}

}  // namespace postgres
}  // namespace storages
