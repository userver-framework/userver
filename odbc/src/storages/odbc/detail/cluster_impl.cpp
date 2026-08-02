#include <storages/odbc/detail/cluster_impl.hpp>

#include <algorithm>
#include <mutex>

#include <storages/odbc/detail/deadline.hpp>
#include <storages/odbc/detail/pool.hpp>
#include <storages/odbc/detail/statement_stats.hpp>
#include <storages/odbc/detail/topology/topology_base.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/storages/odbc/cluster_types.hpp>
#include <userver/storages/odbc/command_control.hpp>
#include <userver/storages/odbc/exception.hpp>
#include <userver/storages/odbc/impl/tracing_tags.hpp>

#include <userver/testsuite/testpoint.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/datetime/steady_coarse_clock.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

ClusterImpl::ClusterImpl(
    const settings::ODBCClusterSettings& settings,
    clients::dns::Resolver* resolver,
    engine::TaskProcessor& blocking_task_processor
)
    : resolver_{resolver},
      blocking_task_processor_{blocking_task_processor},
      settings_{std::make_shared<const settings::ODBCClusterSettings>(settings)},
      baseline_settings_{settings_},
      command_control_store_{std::make_shared<CommandControlStore>()}
{
    UINVARIANT(!settings.pools.empty(), "Pools count should be positive");
    std::atomic_store(
        &topology_,
        topology::TopologyBase::Create(
            settings,
            statement_metrics_settings_,
            prepared_statement_cache_settings_effective_,
            resolver_,
            blocking_task_processor_
        )
    );
}

ResultSet ClusterImpl::Execute(
    ClusterHostTypeFlags flags,
    OptionalCommandControl command_control,
    const Query& query,
    const impl::ParameterList& parameters
) {
    const auto resolved = ResolveCommandControl(command_control, query.GetOptionalNameView());
    return ExecuteImpl(
        GetExecuteDeadline(resolved.network_timeout.value_or(kDefaultStatementTimeout)),
        resolved.statement_timeout.value_or(kDefaultStatementTimeout),
        flags,
        query,
        parameters
    );
}

ResultSet ClusterImpl::ExecuteImpl(
    engine::Deadline acquire_deadline,
    std::chrono::milliseconds statement_timeout,
    ClusterHostTypeFlags flags,
    const Query& query,
    const impl::ParameterList& parameters
) {
    CheckDeadlineNotExpired(acquire_deadline);

    tracing::Span span{storages::odbc::impl::tracing::kExecuteSpan};
    const auto topology = std::atomic_load(&topology_);
    auto& pool = SelectPool(*topology, flags);
    auto conn = pool.Acquire(acquire_deadline);

    pool.AccountOutOfTransaction();
    const auto statement_deadline = GetExecuteDeadline(statement_timeout);

    const auto start = utils::datetime::SteadyCoarseClock::now();
    StatementStats statement_stats{query, pool.GetStatementStatsStorage()};
    try {
        CheckDeadlineNotExpired(statement_deadline);
        auto result = conn->Query(query, parameters, std::min(acquire_deadline, statement_deadline));
        const auto elapsed = std::chrono::duration_cast<
            std::chrono::microseconds>(utils::datetime::SteadyCoarseClock::now() - start);
        pool.AccountQueryExecuted(elapsed);
        statement_stats.AccountSuccess();
        return result;
    } catch (const OperationInterrupted&) {
        statement_stats.AccountError();
        pool.AccountQueryTimeout();
        throw;
    } catch (const Error&) {
        statement_stats.AccountError();
        pool.AccountQueryError();
        throw;
    } catch (const std::exception&) {
        statement_stats.AccountError();
        throw;
    }
}

Transaction ClusterImpl::Begin(ClusterHostTypeFlags flags) { return Begin(flags, TransactionOptions{}, std::nullopt); }

Transaction ClusterImpl::Begin(ClusterHostTypeFlags flags, OptionalCommandControl command_control) {
    return Begin(flags, TransactionOptions{}, command_control);
}

Transaction ClusterImpl::Begin(ClusterHostTypeFlags flags, const TransactionOptions& options) {
    return Begin(flags, options, std::nullopt);
}

Transaction ClusterImpl::Begin(
    ClusterHostTypeFlags flags,
    const TransactionOptions& options,
    OptionalCommandControl command_control
) {
    const auto resolved = ResolveCommandControl(command_control, std::nullopt);
    const auto network_timeout = resolved.network_timeout.value_or(kDefaultStatementTimeout);
    const auto statement_timeout = resolved.statement_timeout.value_or(kDefaultStatementTimeout);
    const auto acquire_deadline = GetExecuteDeadline(network_timeout);
    CheckDeadlineNotExpired(acquire_deadline);

    tracing::Span span{storages::odbc::impl::tracing::kTransactionSpan};
    const auto topology = std::atomic_load(&topology_);
    auto& pool = SelectPool(*topology, flags);
    auto connection = pool.Acquire(acquire_deadline);
    connection->SetCommandControlStore(command_control_store_);
    return Transaction{std::move(connection), pool, options, network_timeout, statement_timeout};
}

CommandControl ClusterImpl::ResolveCommandControl(
    OptionalCommandControl command_control,
    std::optional<Query::NameView> query_name
) const {
    const auto* task_data = server::request::kTaskInheritedData.GetOptional();
    if (task_data) {
        return command_control_store_->Resolve(task_data->path, task_data->method, query_name, command_control);
    }
    return command_control_store_->Resolve(std::nullopt, std::nullopt, query_name, command_control);
}

Pool& ClusterImpl::SelectPool(const topology::TopologyBase& topology, ClusterHostTypeFlags flags) {
    if (flags & ClusterHostType::kSlave) {
        return topology.SelectPool(ClusterHostType::kSlave);
    }

    // kMaster + kNone go to primary
    return topology.SelectPool(ClusterHostType::kMaster);
}

void ClusterImpl::WriteStatistics(utils::statistics::Writer& writer) const {
    const auto topology = std::atomic_load(&topology_);
    UASSERT(topology);
    topology->WriteStatistics(writer);
}

void ClusterImpl::UpdateSettings(const settings::ODBCClusterSettings& settings) {
    UINVARIANT(!settings.pools.empty(), "Pools count should be positive");
    bool updated = false;
    {
        const std::lock_guard lock{settings_mutex_};
        baseline_settings_ = std::make_shared<const settings::ODBCClusterSettings>(settings);
        updated = UpdateSettingsLocked(MakeEffectiveSettingsLocked());
    }
    if (updated) {
        TESTPOINT("odbc-new-dsn-list", {});
    }
}

bool ClusterImpl::UpdateSettingsLocked(const settings::ODBCClusterSettings& settings) {
    if (settings_ && *settings_ == settings) {
        return false;
    }

    // Construct and initialize all new pools before publishing. If this throws,
    // the currently working topology remains untouched.
    auto new_topology = topology::TopologyBase::Create(
        settings,
        statement_metrics_settings_,
        prepared_statement_cache_settings_effective_,
        resolver_,
        blocking_task_processor_
    );
    auto new_settings = std::make_shared<const settings::ODBCClusterSettings>(settings);
    settings_ = std::move(new_settings);
    std::atomic_store(&topology_, std::move(new_topology));
    return true;
}

void ClusterImpl::UpdateDsns(const std::vector<std::string>& dsns) {
    UINVARIANT(!dsns.empty(), "ODBC DSN list must not be empty");
    bool updated = false;
    {
        const std::lock_guard lock{settings_mutex_};
        UASSERT(baseline_settings_ && !baseline_settings_->pools.empty());

        const bool dsns_unchanged =
            dsns.size() == baseline_settings_->pools.size() &&
            std::equal(
                dsns.begin(),
                dsns.end(),
                baseline_settings_->pools.begin(),
                [](const std::string& dsn, const settings::HostSettings& host) { return dsn == host.dsn; }
            );

        if (!dsns_unchanged) {
            const auto pool_settings = baseline_settings_->pools.front().pool;
            settings::ODBCClusterSettings updated_baseline;
            updated_baseline.pools.reserve(dsns.size());
            for (const auto& dsn : dsns) {
                updated_baseline.pools.emplace_back(settings::HostSettings{.dsn = dsn, .pool = pool_settings});
            }
            baseline_settings_ = std::make_shared<const settings::ODBCClusterSettings>(std::move(updated_baseline));
        }
        updated = UpdateSettingsLocked(MakeEffectiveSettingsLocked());
    }
    if (updated) {
        TESTPOINT("odbc-new-dsn-list", {});
    }
}

void ClusterImpl::SetPoolSettingsOverride(std::optional<settings::PoolSettings> pool_settings) {
    {
        const std::lock_guard lock{settings_mutex_};
        if (pool_settings_override_ != pool_settings) {
            pool_settings_override_ = pool_settings;
        }
        UpdateSettingsLocked(MakeEffectiveSettingsLocked());
    }
}

settings::ODBCClusterSettings ClusterImpl::MakeEffectiveSettingsLocked() const {
    UASSERT(baseline_settings_);
    settings::ODBCClusterSettings effective;
    effective.pools.reserve(baseline_settings_->pools.size());
    for (const auto& host : baseline_settings_->pools) {
        effective.pools.emplace_back(settings::HostSettings{
            .dsn = host.dsn,
            .pool = pool_settings_override_.value_or(host.pool),
        });
    }
    return effective;
}

void ClusterImpl::SetDefaultCommandControl(const CommandControl& cc) { command_control_store_->SetDefault(cc); }

void ClusterImpl::SetHandlersCommandControl(CommandControlByHandlerMap command_control) {
    command_control_store_->SetHandlers(std::move(command_control));
}

void ClusterImpl::SetQueriesCommandControl(CommandControlByQueryMap command_control) {
    command_control_store_->SetQueries(std::move(command_control));
}

void ClusterImpl::SetStatementMetricsSettings(const settings::StatementMetricsSettings& settings) {
    const std::lock_guard lock{settings_mutex_};
    if (statement_metrics_settings_ == settings) {
        return;
    }
    statement_metrics_settings_ = settings;
    const auto topology = std::atomic_load(&topology_);
    UASSERT(topology);
    topology->SetStatementMetricsSettings(settings);
}

void ClusterImpl::SetPreparedStatementCacheSettings(const settings::PreparedStatementCacheSettings& settings) {
    const std::lock_guard lock{settings_mutex_};
    prepared_statement_cache_settings_baseline_ = settings;
    ApplyPreparedStatementCacheSettingsLocked();
}

void ClusterImpl::SetPreparedStatementCacheSettingsOverride(
    std::optional<settings::PreparedStatementCacheSettings> settings
) {
    const std::lock_guard lock{settings_mutex_};
    prepared_statement_cache_settings_override_ = settings;
    ApplyPreparedStatementCacheSettingsLocked();
}

void ClusterImpl::ApplyPreparedStatementCacheSettingsLocked() {
    const auto
        effective = prepared_statement_cache_settings_override_.value_or(prepared_statement_cache_settings_baseline_);
    if (effective == prepared_statement_cache_settings_effective_) {
        return;
    }
    prepared_statement_cache_settings_effective_ = effective;
    const auto topology = std::atomic_load(&topology_);
    UASSERT(topology);
    topology->SetPreparedStatementCacheSettings(effective);
}

void ClusterImpl::ApplyDynamicCommandControls(
    CommandControl default_command_control,
    CommandControlByHandlerMap handlers_command_control,
    CommandControlByQueryMap queries_command_control
) {
    command_control_store_->Assign(CommandControlConfig{
        .default_command_control = std::move(default_command_control),
        .handlers_command_control = std::move(handlers_command_control),
        .queries_command_control = std::move(queries_command_control),
    });
}

std::optional<std::chrono::milliseconds> ClusterImpl::GetDefaultNetworkTimeout() const {
    return command_control_store_->GetDefault().network_timeout;
}

std::optional<std::chrono::milliseconds> ClusterImpl::GetDefaultStatementTimeout() const {
    return command_control_store_->GetDefault().statement_timeout;
}

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
