#include <userver/storages/postgres/cluster.hpp>

#include <storages/postgres/detail/cluster_impl.hpp>
#include <storages/postgres/detail/pg_impl_types.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

Cluster::Cluster(DsnList dsns, clients::dns::Resolver* resolver,
                 engine::TaskProcessor& bg_task_processor,
                 const ClusterSettings& cluster_settings,
                 DefaultCommandControls&& default_cmd_ctls,
                 const testsuite::PostgresControl& testsuite_pg_ctl,
                 const error_injection::Settings& ei_settings,
                 testsuite::TestsuiteTasks& testsuite_tasks,
                 dynamic_config::Source config_source) {
  pimpl_ = std::make_unique<detail::ClusterImpl>(
      std::move(dsns), resolver, bg_task_processor, cluster_settings,
      std::move(default_cmd_ctls), testsuite_pg_ctl, ei_settings,
      testsuite_tasks, std::move(config_source));
}

Cluster::~Cluster() = default;

ClusterStatisticsPtr Cluster::GetStatistics() const {
  return pimpl_->GetStatistics();
}

Transaction Cluster::Begin(const TransactionOptions& options,
                           OptionalCommandControl cmd_ctl) {
  return Begin({}, options, cmd_ctl);
}

Transaction Cluster::Begin(ClusterHostTypeFlags flags,
                           const TransactionOptions& options,
                           OptionalCommandControl cmd_ctl) {
  return pimpl_->Begin(flags, options, GetHandlersCmdCtl(cmd_ctl));
}

Transaction Cluster::Begin(const std::string& name,
                           const TransactionOptions& options) {
  return Begin(name, {}, options);
}

Transaction Cluster::Begin(const std::string& name, ClusterHostTypeFlags flags,
                           const TransactionOptions& options) {
  return pimpl_->Begin(flags, options, GetHandlersCmdCtl(GetQueryCmdCtl(name)));
}

void Cluster::SetDefaultCommandControl(CommandControl cmd_ctl) {
  pimpl_->SetDefaultCommandControl(cmd_ctl,
                                   detail::DefaultCommandControlSource::kUser);
}

CommandControl Cluster::GetDefaultCommandControl() const {
  return pimpl_->GetDefaultCommandControl();
}

void Cluster::SetHandlersCommandControl(
    const CommandControlByHandlerMap& handlers_command_control) {
  pimpl_->SetHandlersCommandControl(handlers_command_control);
}

void Cluster::SetQueriesCommandControl(
    const CommandControlByQueryMap& queries_command_control) {
  pimpl_->SetQueriesCommandControl(queries_command_control);
}

void Cluster::ApplyGlobalCommandControlUpdate(CommandControl cmd_ctl) {
  pimpl_->SetDefaultCommandControl(
      cmd_ctl, detail::DefaultCommandControlSource::kGlobalConfig);
}

void Cluster::SetConnectionSettings(const ConnectionSettings& settings) {
  pimpl_->SetConnectionSettings(settings);
}

void Cluster::SetPoolSettings(const PoolSettings& settings) {
  pimpl_->SetPoolSettings(settings);
}

void Cluster::SetStatementMetricsSettings(
    const StatementMetricsSettings& settings) {
  pimpl_->SetStatementMetricsSettings(settings);
}

detail::NonTransaction Cluster::Start(ClusterHostTypeFlags flags,
                                      OptionalCommandControl cmd_ctl) {
  return pimpl_->Start(flags, cmd_ctl);
}

OptionalCommandControl Cluster::GetQueryCmdCtl(
    const std::string& query_name) const {
  return pimpl_->GetQueryCmdCtl(query_name);
}

OptionalCommandControl Cluster::GetHandlersCmdCtl(
    OptionalCommandControl cmd_ctl) const {
  return cmd_ctl ? cmd_ctl : pimpl_->GetTaskDataHandlersCommandControl();
}

ResultSet Cluster::Execute(ClusterHostTypeFlags flags, const Query& query,
                           const ParameterStore& store) {
  return Execute(flags, OptionalCommandControl{}, query, store);
}

ResultSet Cluster::Execute(ClusterHostTypeFlags flags,
                           OptionalCommandControl statement_cmd_ctl,
                           const Query& query, const ParameterStore& store) {
  if (!statement_cmd_ctl && query.GetName()) {
    statement_cmd_ctl = GetQueryCmdCtl(query.GetName()->GetUnderlying());
  }
  statement_cmd_ctl = GetHandlersCmdCtl(statement_cmd_ctl);
  auto ntrx = Start(flags, statement_cmd_ctl);
  return ntrx.Execute(statement_cmd_ctl, query.Statement(), store);
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
