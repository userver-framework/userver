#include <storages/postgres/detail/cluster_impl.hpp>

#include <fmt/format.h>

#include <userver/dynamic_config/value.hpp>
#include <userver/engine/async.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/utils/assert.hpp>

#include <storages/postgres/detail/topology/hot_standby.hpp>
#include <storages/postgres/detail/topology/standalone.hpp>
#include <storages/postgres/postgres_config.hpp>
#include <userver/storages/postgres/dsn.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/utils/impl/userver_experiments.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

namespace {

USERVER_NAMESPACE::utils::impl::UserverExperiment kConnlimitAutoExperiment(
    "pg-connlimit-auto");

ClusterHostType Fallback(ClusterHostType ht) {
  switch (ht) {
    case ClusterHostType::kMaster:
      throw ClusterError("Cannot fallback from master");
    case ClusterHostType::kSyncSlave:
    case ClusterHostType::kSlave:
      return ClusterHostType::kMaster;
    case ClusterHostType::kSlaveOrMaster:
    case ClusterHostType::kNone:
    case ClusterHostType::kRoundRobin:
    case ClusterHostType::kNearest:
      throw ClusterError("Invalid ClusterHostType value for fallback " +
                         ToString(ht));
  }
  UINVARIANT(false, "Unexpected cluster host type");
}

size_t SelectDsnIndex(const topology::TopologyBase::DsnIndices& indices,
                      ClusterHostTypeFlags flags,
                      std::atomic<uint32_t>& rr_host_idx) {
  UASSERT(!indices.empty());
  if (indices.empty()) {
    throw ClusterError("Cannot select host from an empty list");
  }

  const auto strategy_flags = flags & kClusterHostStrategyMask;
  LOG_TRACE() << "Applying " << strategy_flags << " strategy";

  size_t idx_pos = 0;
  if (!strategy_flags || strategy_flags == ClusterHostType::kRoundRobin) {
    if (indices.size() != 1) {
      idx_pos =
          rr_host_idx.fetch_add(1, std::memory_order_relaxed) % indices.size();
    }
  } else if (strategy_flags != ClusterHostType::kNearest) {
    throw LogicError(
        fmt::format("Invalid strategy requested: {}, ensure only one is used",
                    ToString(strategy_flags)));
  }
  return indices[idx_pos];
}

}  // namespace

ClusterImpl::ClusterImpl(DsnList dsns, clients::dns::Resolver* resolver,
                         engine::TaskProcessor& bg_task_processor,
                         const ClusterSettings& cluster_settings,
                         const DefaultCommandControls& default_cmd_ctls,
                         const testsuite::PostgresControl& testsuite_pg_ctl,
                         const error_injection::Settings& ei_settings,
                         testsuite::TestsuiteTasks& testsuite_tasks,
                         dynamic_config::Source config_source)
    : default_cmd_ctls_(default_cmd_ctls),
      cluster_settings_(cluster_settings),
      bg_task_processor_(bg_task_processor),
      rr_host_idx_(0),
      config_source_(std::move(config_source)),
      connlimit_watchdog_(*this, testsuite_tasks,
                          [this]() { OnConnlimitChanged(); }) {
  if (dsns.empty()) {
    throw ClusterError("Cannot create a cluster from an empty DSN list");
  } else if (dsns.size() == 1) {
    LOG_INFO() << "Creating a cluster in standalone mode";
    topology_ = std::make_unique<topology::Standalone>(
        bg_task_processor, std::move(dsns), resolver,
        cluster_settings.topology_settings, cluster_settings.conn_settings,
        default_cmd_ctls_, testsuite_pg_ctl, ei_settings);
  } else {
    LOG_INFO() << "Creating a cluster in hot standby mode";
    topology_ = std::make_unique<topology::HotStandby>(
        bg_task_processor, std::move(dsns), resolver,
        cluster_settings.topology_settings, cluster_settings.conn_settings,
        default_cmd_ctls_, testsuite_pg_ctl, ei_settings);
  }

  UASSERT(topology_);
  const auto& dsn_list = topology_->GetDsnList();
  UASSERT(!dsn_list.empty());

  LOG_DEBUG() << "Starting pools initialization";
  host_pools_.reserve(dsn_list.size());
  for (const auto& dsn : dsn_list) {
    host_pools_.push_back(ConnectionPool::Create(
        dsn, resolver, bg_task_processor_, cluster_settings.db_name,
        cluster_settings.init_mode, cluster_settings.pool_settings,
        cluster_settings.conn_settings,
        cluster_settings.statement_metrics_settings, default_cmd_ctls_,
        testsuite_pg_ctl, ei_settings));
  }
  LOG_DEBUG() << "Pools initialized";

  // Do not use IsConnlimitModeAuto() here because we don't care about
  // the current dynamic config value
  if (cluster_settings.connlimit_mode == ConnlimitMode::kAuto &&
      kConnlimitAutoExperiment.IsEnabled()) {
    connlimit_watchdog_.Start();
  }
}

ClusterImpl::~ClusterImpl() { connlimit_watchdog_.Stop(); }

ClusterStatisticsPtr ClusterImpl::GetStatistics() const {
  auto cluster_stats = std::make_unique<ClusterStatistics>();

  const auto& dsns = topology_->GetDsnList();
  std::vector<int8_t> is_host_pool_seen(dsns.size(), 0);
  auto dsn_indices_by_type = topology_->GetDsnIndicesByType();
  const auto& dsn_stats = topology_->GetDsnStatistics();

  UASSERT(host_pools_.size() == dsns.size());

  auto master_dsn_indices_it =
      dsn_indices_by_type->find(ClusterHostType::kMaster);
  if (master_dsn_indices_it != dsn_indices_by_type->end() &&
      !master_dsn_indices_it->second.empty()) {
    auto dsn_index = master_dsn_indices_it->second.front();
    UASSERT(dsn_index < dsns.size());
    cluster_stats->master.host_port = GetHostPort(dsns[dsn_index]);
    UASSERT(dsn_index < host_pools_.size());
    UASSERT(dsn_index < dsn_stats.size());
    cluster_stats->master.stats.Add(host_pools_[dsn_index]->GetStatistics(),
                                    dsn_stats[dsn_index]);
    cluster_stats->master.stats.Add(host_pools_[dsn_index]
                                        ->GetStatementTimingsStorage()
                                        .GetTimingsPercentiles());
    is_host_pool_seen[dsn_index] = 1;
  }

  auto sync_slave_dsn_indices_it =
      dsn_indices_by_type->find(ClusterHostType::kSyncSlave);
  if (sync_slave_dsn_indices_it != dsn_indices_by_type->end() &&
      !sync_slave_dsn_indices_it->second.empty()) {
    auto dsn_index = sync_slave_dsn_indices_it->second.front();
    UASSERT(dsn_index < dsns.size());
    cluster_stats->sync_slave.host_port = GetHostPort(dsns[dsn_index]);
    UASSERT(dsn_index < host_pools_.size());
    UASSERT(dsn_index < dsn_stats.size());
    cluster_stats->sync_slave.stats.Add(host_pools_[dsn_index]->GetStatistics(),
                                        dsn_stats[dsn_index]);
    cluster_stats->sync_slave.stats.Add(host_pools_[dsn_index]
                                            ->GetStatementTimingsStorage()
                                            .GetTimingsPercentiles());
    is_host_pool_seen[dsn_index] = 1;
  }

  auto slaves_dsn_indices_it =
      dsn_indices_by_type->find(ClusterHostType::kSlave);
  if (slaves_dsn_indices_it != dsn_indices_by_type->end() &&
      !slaves_dsn_indices_it->second.empty()) {
    cluster_stats->slaves.reserve(slaves_dsn_indices_it->second.size());
    for (auto dsn_index : slaves_dsn_indices_it->second) {
      if (is_host_pool_seen[dsn_index]) continue;

      auto& slave_desc = cluster_stats->slaves.emplace_back();
      UASSERT(dsn_index < dsns.size());
      slave_desc.host_port = GetHostPort(dsns[dsn_index]);
      UASSERT(dsn_index < host_pools_.size());
      UASSERT(dsn_index < dsn_stats.size());
      slave_desc.stats.Add(host_pools_[dsn_index]->GetStatistics(),
                           dsn_stats[dsn_index]);
      slave_desc.stats.Add(host_pools_[dsn_index]
                               ->GetStatementTimingsStorage()
                               .GetTimingsPercentiles());
      is_host_pool_seen[dsn_index] = 1;
    }
  }
  for (size_t i = 0; i < is_host_pool_seen.size(); ++i) {
    if (is_host_pool_seen[i]) continue;

    auto& desc = cluster_stats->unknown.emplace_back();
    UASSERT(i < dsns.size());
    desc.host_port = GetHostPort(dsns[i]);
    UASSERT(i < host_pools_.size());
    UASSERT(i < dsn_stats.size());
    desc.stats.Add(host_pools_[i]->GetStatistics(), dsn_stats[i]);
    desc.stats.Add(
        host_pools_[i]->GetStatementTimingsStorage().GetTimingsPercentiles());

    cluster_stats->unknown.push_back(std::move(desc));
  }

  return cluster_stats;
}

ClusterImpl::ConnectionPoolPtr ClusterImpl::FindPool(
    ClusterHostTypeFlags flags) {
  LOG_TRACE() << "Looking for pool: " << flags;

  size_t dsn_index = -1;
  const auto role_flags = flags & kClusterHostRolesMask;

  UASSERT_MSG(role_flags, "No roles specified");
  UASSERT_MSG(!(role_flags & ClusterHostType::kSyncSlave) ||
                  role_flags == ClusterHostType::kSyncSlave,
              "kSyncSlave cannot be combined with other roles");

  if ((role_flags & ClusterHostType::kMaster) &&
      (role_flags & ClusterHostType::kSlave)) {
    LOG_TRACE() << "Starting transaction on " << role_flags;
    auto alive_dsn_indices = topology_->GetAliveDsnIndices();
    if (alive_dsn_indices->empty()) {
      throw ClusterUnavailable("None of cluster hosts are available");
    }
    dsn_index = SelectDsnIndex(*alive_dsn_indices, flags, rr_host_idx_);
  } else {
    auto host_role = static_cast<ClusterHostType>(role_flags.GetValue());
    auto dsn_indices_by_type = topology_->GetDsnIndicesByType();
    auto dsn_indices_it = dsn_indices_by_type->find(host_role);
    while (host_role != ClusterHostType::kMaster &&
           (dsn_indices_it == dsn_indices_by_type->end() ||
            dsn_indices_it->second.empty())) {
      auto fb = Fallback(host_role);
      LOG_WARNING() << "There is no pool for " << host_role
                    << ", falling back to " << fb;
      host_role = fb;
      dsn_indices_it = dsn_indices_by_type->find(host_role);
    }

    if (dsn_indices_it == dsn_indices_by_type->end() ||
        dsn_indices_it->second.empty()) {
      throw ClusterUnavailable(
          fmt::format("Pool for {} (requested: {}) is not available",
                      ToString(host_role), ToString(role_flags)));
    }
    LOG_TRACE() << "Starting transaction on " << host_role;
    dsn_index = SelectDsnIndex(dsn_indices_it->second, flags, rr_host_idx_);
  }

  UASSERT(dsn_index < host_pools_.size());
  return host_pools_.at(dsn_index);
}

Transaction ClusterImpl::Begin(ClusterHostTypeFlags flags,
                               const TransactionOptions& options,
                               OptionalCommandControl cmd_ctl) {
  LOG_TRACE() << "Requested transaction on " << flags;
  const auto role_flags = flags & kClusterHostRolesMask;
  if (options.IsReadOnly()) {
    if (!role_flags) {
      flags |= ClusterHostType::kSlave;
    }
  } else {
    if (role_flags && !(role_flags & ClusterHostType::kMaster)) {
      throw ClusterUnavailable("Cannot start RW-transaction on a slave");
    }
    flags = ClusterHostType::kMaster | flags.Clear(kClusterHostRolesMask);
  }
  return FindPool(flags)->Begin(options, cmd_ctl);
}

NonTransaction ClusterImpl::Start(ClusterHostTypeFlags flags,
                                  OptionalCommandControl cmd_ctl) {
  if (!(flags & kClusterHostRolesMask)) {
    throw LogicError(
        "Host role must be specified for execution of a single statement");
  }
  LOG_TRACE() << "Requested single statement on " << flags;
  return FindPool(flags)->Start(cmd_ctl);
}

void ClusterImpl::SetDefaultCommandControl(CommandControl cmd_ctl,
                                           DefaultCommandControlSource source) {
  default_cmd_ctls_.UpdateDefaultCmdCtl(cmd_ctl, source);
}

CommandControl ClusterImpl::GetDefaultCommandControl() const {
  return default_cmd_ctls_.GetDefaultCmdCtl();
}

void ClusterImpl::SetHandlersCommandControl(
    const CommandControlByHandlerMap& handlers_command_control) {
  default_cmd_ctls_.UpdateHandlersCommandControl(handlers_command_control);
}

void ClusterImpl::SetQueriesCommandControl(
    const CommandControlByQueryMap& queries_command_control) {
  default_cmd_ctls_.UpdateQueriesCommandControl(queries_command_control);
}

void ClusterImpl::SetConnectionSettings(const ConnectionSettings& settings) {
  for (const auto& pool : host_pools_) {
    pool->SetConnectionSettings(settings);
  }
}

void ClusterImpl::SetPoolSettings(const PoolSettings& new_settings) {
  {
    auto cluster = cluster_settings_.StartWrite();

    cluster->pool_settings = new_settings;
    auto& settings = cluster->pool_settings;
    if (IsConnlimitModeAuto(*cluster)) {
      settings.max_size = connlimit_watchdog_.GetConnlimit();
      if (settings.min_size > settings.max_size)
        settings.min_size = settings.max_size;
    }

    cluster.Commit();
  }

  auto cluster_settings = cluster_settings_.Read();
  for (const auto& pool : host_pools_) {
    pool->SetSettings(cluster_settings->pool_settings);
  }
}

void ClusterImpl::OnConnlimitChanged() {
  auto max_size = connlimit_watchdog_.GetConnlimit();
  auto cluster = cluster_settings_.StartWrite();
  if (!IsConnlimitModeAuto(*cluster)) return;

  if (cluster->pool_settings.max_size == max_size) return;
  cluster->pool_settings.max_size = max_size;
  cluster.Commit();

  auto cluster_settings = cluster_settings_.Read();
  SetPoolSettings(cluster_settings->pool_settings);
}

bool ClusterImpl::IsConnlimitModeAuto(const ClusterSettings& settings) const {
  if (settings.connlimit_mode == ConnlimitMode::kManual) return false;

  if (!kConnlimitAutoExperiment.IsEnabled()) return false;

  auto snapshot = config_source_.GetSnapshot();
  // NOLINTNEXTLINE(readability-simplify-boolean-expr)
  if (!snapshot[kConnlimitConfig].connlimit_mode_auto_enabled) return false;

  return true;
}

void ClusterImpl::SetStatementMetricsSettings(
    const StatementMetricsSettings& settings) {
  for (const auto& pool : host_pools_) {
    pool->SetStatementMetricsSettings(settings);
  }
}

OptionalCommandControl ClusterImpl::GetQueryCmdCtl(
    const std::string& query_name) const {
  return default_cmd_ctls_.GetQueryCmdCtl(query_name);
}

OptionalCommandControl ClusterImpl::GetTaskDataHandlersCommandControl() const {
  const auto* task_data = server::request::kTaskInheritedData.GetOptional();
  if (task_data && task_data->path) {
    return default_cmd_ctls_.GetHandlerCmdCtl(*task_data->path,
                                              task_data->method);
  }
  return std::nullopt;
}

std::string ClusterImpl::GetDbName() const {
  auto cluster_settings = cluster_settings_.Read();
  return cluster_settings->db_name;
}

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
