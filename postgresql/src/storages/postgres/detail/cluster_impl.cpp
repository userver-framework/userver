#include <storages/postgres/detail/cluster_impl.hpp>

#include <algorithm>

#include <userver/dynamic_config/value.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>

#include <storages/postgres/detail/host_selection.hpp>
#include <storages/postgres/detail/topology/hot_standby.hpp>
#include <storages/postgres/detail/topology/standalone.hpp>
#include <userver/storages/postgres/dsn.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/utils/impl/userver_experiments.hpp>

#include <dynamic_config/variables/POSTGRES_CONNLIMIT_MODE_AUTO_ENABLED.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

namespace {

// One connection per host is enough to register a new instance in u_clients.
constexpr std::size_t kBootstrapPoolSize = 1;

}  // namespace

ClusterImpl::ClusterImpl(
    DsnList dsns,
    clients::dns::Resolver* resolver,
    engine::TaskProcessor& bg_task_processor,
    const ClusterSettings& cluster_settings,
    const DefaultCommandControls& default_cmd_ctls,
    const testsuite::PostgresControl& testsuite_pg_ctl,
    const error_injection::Settings& ei_settings,
    testsuite::TestsuiteTasks& testsuite_tasks,
    dynamic_config::Source config_source,
    USERVER_NAMESPACE::utils::statistics::MetricsStoragePtr metrics,
    int shard_number
)
    : cluster_settings_(cluster_settings),
      resolver_(resolver),
      bg_task_processor_(bg_task_processor),
      config_source_(std::move(config_source)),
      default_cmd_ctls_(default_cmd_ctls),
      testsuite_pg_ctl_(testsuite_pg_ctl),
      ei_settings_(ei_settings),
      metrics_(std::move(metrics)),
      rr_host_idx_(0),
      connlimit_watchdog_(
          *this,
          testsuite_tasks,
          shard_number,
          cluster_settings.pool_settings.min_size,
          [this]() { OnConnlimitChanged(); },
          dsns.size() > 1 ? 1 : 0
      )
{
    CreateTopology(dsns);

    // Do not use IsConnlimitModeAuto() here because we don't care about
    // the current dynamic config value
    if (cluster_settings.connlimit_mode == ConnlimitMode::kAuto) {
        connlimit_watchdog_.Start();
    }
}

void ClusterImpl::CreateTopology(const DsnList& dsns) {
    TopologyData data;

    auto cluster_settings = cluster_settings_.Read();

    if (dsns.empty()) {
        throw ClusterError("Cannot create a cluster from an empty DSN list");
    } else if (dsns.size() == 1) {
        LOG_INFO() << "Creating a cluster in standalone mode";
        data.topology = std::make_unique<topology::Standalone>(
            bg_task_processor_,
            std::move(dsns),
            resolver_,
            cluster_settings->topology_settings,
            cluster_settings->conn_settings,
            default_cmd_ctls_,
            testsuite_pg_ctl_,
            ei_settings_,
            metrics_
        );
    } else {
        LOG_INFO() << "Creating a cluster in hot standby mode";
        data.topology = std::make_unique<topology::HotStandby>(
            bg_task_processor_,
            std::move(dsns),
            resolver_,
            cluster_settings->topology_settings,
            cluster_settings->conn_settings,
            default_cmd_ctls_,
            testsuite_pg_ctl_,
            ei_settings_,
            metrics_
        );
    }

    auto existing_td = topology_data_.UniqueLock();
    std::unordered_map<std::string, ConnectionPoolPtr> existing_pools_by_name;
    for (const auto& pool : existing_td->host_pools) {
        existing_pools_by_name.emplace(pool->GetDsn(), pool);
    }

    LOG_DEBUG() << "Starting pools initialization";
    const auto& dsn_list = data.topology->GetDsnList();
    const auto startup_pool_settings = MakeEffectivePoolSettings(*cluster_settings);
    UASSERT(!dsn_list.empty());
    data.host_pools.reserve(dsn_list.size());
    for (const auto& dsn : dsn_list) {
        auto pool = USERVER_NAMESPACE::utils::FindOrNullptr(existing_pools_by_name, dsn.GetUnderlying());
        if (pool) {
            data.host_pools.push_back(std::move(*pool));
        } else {
            data.host_pools.push_back(ConnectionPool::Create(
                dsn,
                resolver_,
                bg_task_processor_,
                cluster_settings->db_name,
                cluster_settings->init_mode,
                startup_pool_settings,
                cluster_settings->conn_settings,
                cluster_settings->statement_metrics_settings,
                default_cmd_ctls_,
                testsuite_pg_ctl_,
                ei_settings_,
                cluster_settings->cc_config,
                config_source_,
                metrics_
            ));
        }
    }
    LOG_DEBUG() << "Pools initialized";

    *existing_td = std::move(data);
}

ClusterImpl::~ClusterImpl() { connlimit_watchdog_.Stop(); }

ClusterStatisticsPtr ClusterImpl::GetStatistics() const {
    auto cluster_stats = std::make_unique<ClusterStatistics>();

    {
        const auto cluster_settings = cluster_settings_.Read();
        cluster_stats->connlimit_mode_auto_on = IsConnlimitModeAuto(*cluster_settings);
    }

    auto topology_data = topology_data_.SharedLock();
    auto* topology = &*topology_data->topology;
    auto& host_pools = topology_data->host_pools;

    const auto& dsns = topology->GetDsnList();
    std::vector<int8_t> is_host_pool_seen(dsns.size(), 0);
    auto dsn_indices_by_type = topology->GetDsnIndicesByType();
    const auto& dsn_stats = topology->GetDsnStatistics();

    UASSERT(host_pools.size() == dsns.size());

    auto master_dsn_indices_it = dsn_indices_by_type->find(ClusterHostType::kMaster);
    if (master_dsn_indices_it != dsn_indices_by_type->end() && !master_dsn_indices_it->second.indices.empty()) {
        auto dsn_index = master_dsn_indices_it->second.indices.front();
        UASSERT(dsn_index < dsns.size());
        cluster_stats->master.host_port = GetHostPort(dsns[dsn_index]);
        UASSERT(dsn_index < host_pools.size());
        UASSERT(dsn_index < dsn_stats.size());
        cluster_stats->master.stats.Add(host_pools[dsn_index]->GetStatistics(), dsn_stats[dsn_index]);
        cluster_stats->master.stats.Add(host_pools[dsn_index]->GetStatementStatsStorage().GetStatementsStats());
        is_host_pool_seen[dsn_index] = 1;
    }

    auto sync_slave_dsn_indices_it = dsn_indices_by_type->find(ClusterHostType::kSyncSlave);
    if (sync_slave_dsn_indices_it != dsn_indices_by_type->end() && !sync_slave_dsn_indices_it->second.indices.empty()) {
        auto dsn_index = sync_slave_dsn_indices_it->second.indices.front();
        UASSERT(dsn_index < dsns.size());
        cluster_stats->sync_slave.host_port = GetHostPort(dsns[dsn_index]);
        UASSERT(dsn_index < host_pools.size());
        UASSERT(dsn_index < dsn_stats.size());
        cluster_stats->sync_slave.stats.Add(host_pools[dsn_index]->GetStatistics(), dsn_stats[dsn_index]);
        cluster_stats->sync_slave.stats.Add(host_pools[dsn_index]->GetStatementStatsStorage().GetStatementsStats());
        is_host_pool_seen[dsn_index] = 1;
    }

    auto slaves_dsn_indices_it = dsn_indices_by_type->find(ClusterHostType::kSlave);
    if (slaves_dsn_indices_it != dsn_indices_by_type->end() && !slaves_dsn_indices_it->second.indices.empty()) {
        cluster_stats->slaves.reserve(slaves_dsn_indices_it->second.indices.size());
        for (auto dsn_index : slaves_dsn_indices_it->second.indices) {
            if (is_host_pool_seen[dsn_index]) {
                continue;
            }

            auto& slave_desc = cluster_stats->slaves.emplace_back();
            UASSERT(dsn_index < dsns.size());
            slave_desc.host_port = GetHostPort(dsns[dsn_index]);
            UASSERT(dsn_index < host_pools.size());
            UASSERT(dsn_index < dsn_stats.size());
            slave_desc.stats.Add(host_pools[dsn_index]->GetStatistics(), dsn_stats[dsn_index]);
            slave_desc.stats.Add(host_pools[dsn_index]->GetStatementStatsStorage().GetStatementsStats());
            is_host_pool_seen[dsn_index] = 1;
        }
    }
    for (size_t i = 0; i < is_host_pool_seen.size(); ++i) {
        if (is_host_pool_seen[i]) {
            continue;
        }

        auto& desc = cluster_stats->unknown.emplace_back();
        UASSERT(i < dsns.size());
        desc.host_port = GetHostPort(dsns[i]);
        UASSERT(i < host_pools.size());
        UASSERT(i < dsn_stats.size());
        desc.stats.Add(host_pools[i]->GetStatistics(), dsn_stats[i]);
        desc.stats.Add(host_pools[i]->GetStatementStatsStorage().GetStatementsStats());

        cluster_stats->unknown.push_back(std::move(desc));
    }

    return cluster_stats;
}

ClusterImpl::ConnectionPoolPtr ClusterImpl::FindPool(ClusterHostTypeFlags flags) {
    LOG_TRACE() << "Looking for pool: " << flags;

    size_t dsn_index = -1;
    const auto role_flags = flags & kClusterHostRolesMask;

    UASSERT_MSG(role_flags, "No roles specified");
    UASSERT_MSG(
        !(role_flags & ClusterHostType::kSyncSlave) || role_flags == ClusterHostType::kSyncSlave,
        "kSyncSlave cannot be combined with other roles"
    );

    auto td = topology_data_.SharedLock();
    auto& topology = td->topology;
    auto& host_pools = td->host_pools;

    if ((role_flags & ClusterHostType::kMaster) && (role_flags & ClusterHostType::kSlave)) {
        LOG_TRACE() << "Starting transaction on " << role_flags;
        auto alive_dsn_indices = topology->GetAliveDsnIndices();
        if (alive_dsn_indices->indices.empty()) {
            throw ClusterUnavailable("None of cluster hosts are available");
        }
        dsn_index = SelectDsnIndex(*alive_dsn_indices, flags, rr_host_idx_);
    } else {
        const auto requested_role = static_cast<ClusterHostType>(role_flags.GetValue());
        auto dsn_indices_by_type = topology->GetDsnIndicesByType();
        const auto dsn_indices_it = ResolveHostRole(*dsn_indices_by_type, requested_role);
        LOG_TRACE() << "Starting transaction on " << dsn_indices_it->first;
        dsn_index = SelectDsnIndex(dsn_indices_it->second, flags, rr_host_idx_);
    }

    UASSERT(dsn_index < host_pools.size());
    return host_pools.at(dsn_index);
}

Transaction ClusterImpl::Begin(
    ClusterHostTypeFlags flags,
    const TransactionOptions& options,
    OptionalCommandControl cmd_ctl
) {
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

NonTransaction ClusterImpl::Start(ClusterHostTypeFlags flags, OptionalCommandControl cmd_ctl) {
    if (!(flags & kClusterHostRolesMask)) {
        throw LogicError("Host role must be specified for execution of a single statement");
    }
    LOG_TRACE() << "Requested single statement on " << flags;
    return FindPool(flags)->Start(cmd_ctl);
}

NotifyScope ClusterImpl::Listen(std::string_view channel, OptionalCommandControl cmd_ctl) {
    return FindPool(ClusterHostType::kMaster)->Listen(channel, cmd_ctl);
}

QueryQueue ClusterImpl::CreateQueryQueue(ClusterHostTypeFlags flags, TimeoutDuration acquire_timeout) {
    return QueryQueue{
        GetDefaultCommandControl(),
        FindPool(flags)->Acquire(engine::Deadline::FromDuration(acquire_timeout))
    };
}

void ClusterImpl::SetDefaultCommandControl(CommandControl cmd_ctl, DefaultCommandControlSource source) {
    default_cmd_ctls_.UpdateDefaultCmdCtl(cmd_ctl, source);
}

CommandControl ClusterImpl::GetDefaultCommandControl() const { return default_cmd_ctls_.GetDefaultCmdCtl(); }

void ClusterImpl::SetHandlersCommandControl(CommandControlByHandlerMap&& handlers_command_control) {
    default_cmd_ctls_.UpdateHandlersCommandControl(std::move(handlers_command_control));
}

void ClusterImpl::SetQueriesCommandControl(CommandControlByQueryMap&& queries_command_control) {
    default_cmd_ctls_.UpdateQueriesCommandControl(std::move(queries_command_control));
}

void ClusterImpl::SetConnectionSettings(const ConnectionSettings& settings) {
    auto td = topology_data_.SharedLock();

    for (const auto& pool : td->host_pools) {
        pool->SetConnectionSettings(settings);
    }
}

void ClusterImpl::SetPoolSettings(const PoolSettings& new_settings) {
    {
        auto cluster = cluster_settings_.StartWrite();

        cluster->original_min_pool_size = new_settings.min_size;
        cluster->pool_settings = new_settings;
        cluster.Commit();
    }

    PropagateSettingsToPools();
}

PoolSettings ClusterImpl::MakeEffectivePoolSettings(const ExtendedClusterSettings& cluster) const {
    auto pool_settings = cluster.pool_settings;
    if (!IsConnlimitModeAuto(cluster)) {
        return pool_settings;
    }

    const auto connlimit = connlimit_watchdog_.GetConnlimit();
    if (connlimit == 0) {
        pool_settings.min_size = kBootstrapPoolSize;
        pool_settings.max_size = kBootstrapPoolSize;
        return pool_settings;
    }

    pool_settings.min_size = std::min(cluster.original_min_pool_size, connlimit);
    pool_settings.max_size = connlimit;
    return pool_settings;
}

void ClusterImpl::PropagateSettingsToPools() {
    auto td = topology_data_.SharedLock();
    auto cluster_settings = cluster_settings_.Read();
    const auto effective_pool_settings = MakeEffectivePoolSettings(*cluster_settings);
    for (const auto& pool : td->host_pools) {
        pool->SetSettings(effective_pool_settings);
    }
}

void ClusterImpl::SetTopologySettings(const TopologySettings& settings) {
    auto td = topology_data_.SharedLock();
    td->topology->SetTopologySettings(settings);
}

void ClusterImpl::OnConnlimitChanged() {
    {
        auto cluster = cluster_settings_.Read();
        if (!IsConnlimitModeAuto(*cluster)) {
            return;
        }
    }

    PropagateSettingsToPools();
}

bool ClusterImpl::IsConnlimitModeAuto(const ClusterSettings& settings) const {
    if (settings.connlimit_mode == ConnlimitMode::kManual) {
        return false;
    }

    const auto snapshot = config_source_.GetSnapshot();
    return snapshot[::dynamic_config::POSTGRES_CONNLIMIT_MODE_AUTO_ENABLED];
}

void ClusterImpl::SetStatementMetricsSettings(const StatementMetricsSettings& settings) {
    auto td = topology_data_.SharedLock();
    for (const auto& pool : td->host_pools) {
        pool->SetStatementMetricsSettings(settings);
    }
}

OptionalCommandControl ClusterImpl::GetQueryCmdCtl(std::string_view query_name) const {
    return default_cmd_ctls_.GetQueryCmdCtl(query_name);
}

OptionalCommandControl ClusterImpl::GetTaskDataHandlersCommandControl() const {
    const auto* task_data = server::request::kTaskInheritedData.GetOptional();
    if (task_data) {
        return default_cmd_ctls_.GetHandlerCmdCtl(task_data->path, task_data->method);
    }
    return std::nullopt;
}

std::string ClusterImpl::GetDbName() const {
    auto cluster_settings = cluster_settings_.Read();
    return cluster_settings->db_name;
}

void ClusterImpl::SetDsnList(const DsnList& dsn) {
    {
        auto td = topology_data_.SharedLock();
        if (dsn == td->topology->GetDsnList()) {
            return;
        }
    }

    LOG_WARNING() << "Server list has changed for PG " << GetDbName() << ", eventually will drop old sockets";

    CreateTopology(dsn);

    TESTPOINT("postgres-new-dsn-list", {});
}

ClusterImpl::ExtendedClusterSettings::ExtendedClusterSettings(const ClusterSettings& settings)
    : ClusterSettings(settings),
      original_min_pool_size(settings.pool_settings.min_size)
{}

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
