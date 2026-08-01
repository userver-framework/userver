#include <storages/odbc/detail/cluster_impl.hpp>

#include <algorithm>
#include <mutex>

#include <storages/odbc/detail/deadline.hpp>
#include <storages/odbc/detail/pool.hpp>
#include <storages/odbc/detail/topology/topology_base.hpp>
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

static_assert(std::atomic<std::chrono::milliseconds>::is_always_lock_free);

ClusterImpl::ClusterImpl(const settings::ODBCClusterSettings& settings, clients::dns::Resolver* resolver)
    : resolver_{resolver},
      settings_{std::make_shared<const settings::ODBCClusterSettings>(settings)},
      baseline_settings_{settings_}
{
    UINVARIANT(!settings.pools.empty(), "Pools count should be positive");
    std::atomic_store(&topology_, topology::TopologyBase::Create(settings, resolver_));
}

ResultSet ClusterImpl::Execute(
    ClusterHostTypeFlags flags,
    OptionalCommandControl command_control,
    const Query& query,
    const impl::ParameterList& parameters
) {
    return ExecuteImpl(
        GetExecuteDeadline(ResolveNetworkTimeout(command_control)),
        GetExecuteDeadline(ResolveStatementTimeout(command_control)),
        flags,
        query,
        parameters
    );
}

ResultSet ClusterImpl::ExecuteImpl(
    engine::Deadline acquire_deadline,
    engine::Deadline statement_deadline,
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
    CheckDeadlineNotExpired(statement_deadline);

    const auto start = utils::datetime::SteadyCoarseClock::now();
    try {
        auto result = conn->Query(query.GetStatementView(), parameters, std::min(acquire_deadline, statement_deadline));
        const auto elapsed = std::chrono::duration_cast<
            std::chrono::microseconds>(utils::datetime::SteadyCoarseClock::now() - start);
        pool.AccountQueryExecuted(elapsed);
        return result;
    } catch (const OperationInterrupted&) {
        pool.AccountQueryTimeout();
        throw;
    } catch (const Error&) {
        pool.AccountQueryError();
        throw;
    }
}

Transaction ClusterImpl::Begin(ClusterHostTypeFlags flags) { return Begin(flags, std::nullopt); }

Transaction ClusterImpl::Begin(ClusterHostTypeFlags flags, OptionalCommandControl command_control) {
    const auto acquire_deadline = GetExecuteDeadline(ResolveNetworkTimeout(command_control));
    CheckDeadlineNotExpired(acquire_deadline);

    tracing::Span span{storages::odbc::impl::tracing::kTransactionSpan};
    const auto topology = std::atomic_load(&topology_);
    auto& pool = SelectPool(*topology, flags);
    auto connection = pool.Acquire(acquire_deadline);
    const auto statement_deadline = GetExecuteDeadline(ResolveStatementTimeout(command_control));
    return Transaction{std::move(connection), pool, std::min(acquire_deadline, statement_deadline)};
}

std::chrono::milliseconds ClusterImpl::ResolveNetworkTimeout(OptionalCommandControl command_control) const {
    if (command_control && command_control->network_timeout) {
        return *command_control->network_timeout;
    }
    if (const auto configured = GetDefaultNetworkTimeout()) {
        return *configured;
    }
    return kDefaultStatementTimeout;
}

std::chrono::milliseconds ClusterImpl::ResolveStatementTimeout(OptionalCommandControl command_control) const {
    if (command_control && command_control->statement_timeout) {
        return *command_control->statement_timeout;
    }
    if (const auto configured = GetDefaultStatementTimeout()) {
        return *configured;
    }
    return kDefaultStatementTimeout;
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
    auto new_topology = topology::TopologyBase::Create(settings, resolver_);
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

void ClusterImpl::SetDefaultCommandControl(const CommandControl& cc) {
    if (cc.network_timeout.has_value()) {
        default_network_timeout_ms_.store(*cc.network_timeout);
        has_network_timeout_.store(true, std::memory_order_release);
    } else {
        has_network_timeout_.store(false, std::memory_order_release);
    }

    if (cc.statement_timeout.has_value()) {
        default_statement_timeout_ms_.store(*cc.statement_timeout);
        has_statement_timeout_.store(true, std::memory_order_release);
    } else {
        has_statement_timeout_.store(false, std::memory_order_release);
    }
}

std::optional<std::chrono::milliseconds> ClusterImpl::GetDefaultNetworkTimeout() const {
    if (has_network_timeout_.load(std::memory_order_acquire)) {
        return std::chrono::milliseconds{default_network_timeout_ms_.load()};
    }
    return std::nullopt;
}

std::optional<std::chrono::milliseconds> ClusterImpl::GetDefaultStatementTimeout() const {
    if (has_statement_timeout_.load(std::memory_order_acquire)) {
        return std::chrono::milliseconds{default_statement_timeout_ms_.load()};
    }
    return std::nullopt;
}

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
