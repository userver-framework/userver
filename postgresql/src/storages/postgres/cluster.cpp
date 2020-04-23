#include <storages/postgres/cluster.hpp>

#include <storages/postgres/detail/cluster_impl.hpp>
#include <storages/postgres/detail/pg_impl_types.hpp>

namespace storages::postgres {

Cluster::Cluster(DsnList dsns, engine::TaskProcessor& bg_task_processor,
                 const PoolSettings& pool_settings,
                 const ConnectionSettings& conn_settings,
                 const CommandControl& default_cmd_ctl,
                 const testsuite::PostgresControl& testsuite_pg_ctl,
                 const error_injection::Settings& ei_settings) {
  pimpl_ = std::make_unique<detail::ClusterImpl>(
      std::move(dsns), bg_task_processor, pool_settings, conn_settings,
      default_cmd_ctl, testsuite_pg_ctl, ei_settings);
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

detail::NonTransaction Cluster::Start(ClusterHostType ht,
                                      OptionalCommandControl cmd_ctl) {
  return pimpl_->Start(ht, cmd_ctl);
}

void Cluster::SetDefaultCommandControl(CommandControl cmd_ctl) {
  pimpl_->SetDefaultCommandControl(cmd_ctl,
                                   detail::DefaultCommandControlSource::kUser);
}

CommandControl Cluster::GetDefaultCommandControl() const {
  return pimpl_->GetDefaultCommandControl();
}

void Cluster::ApplyGlobalCommandControlUpdate(CommandControl cmd_ctl) {
  pimpl_->SetDefaultCommandControl(
      cmd_ctl, detail::DefaultCommandControlSource::kGlobalConfig);
}

}  // namespace storages::postgres
