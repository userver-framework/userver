#include <storages/odbc/detail/topology/topology_base.hpp>

#include <userver/clients/dns/resolver.hpp>
#include <userver/utils/assert.hpp>

#include <storages/odbc/detail/pool.hpp>
#include <storages/odbc/detail/topology/fixed_primary.hpp>
#include <storages/odbc/detail/topology/standalone.hpp>
#include <storages/odbc/dsn.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail::topology {

namespace {

constexpr std::chrono::seconds kDnsResolveTimeout{5};

std::string ResolveDsn(const std::string& dsn_str, clients::dns::Resolver* resolver) {
    if (!resolver) {
        return dsn_str;
    }
    auto resolved = ResolveDsnHost(Dsn{dsn_str}, *resolver, engine::Deadline::FromDuration(kDnsResolveTimeout));
    return resolved.GetUnderlying();
}

}  // namespace

TopologyBase::TopologyBase(
    const settings::ODBCClusterSettings& settings,
    const settings::StatementMetricsSettings& statement_metrics_settings,
    const settings::PreparedStatementCacheSettings& prepared_statement_cache_settings,
    clients::dns::Resolver* resolver,
    engine::TaskProcessor& blocking_task_processor
) {
    UASSERT(!settings.pools.empty());

    pools_.reserve(settings.pools.size());
    for (const auto& host : settings.pools) {
        auto resolved_dsn = ResolveDsn(host.dsn, resolver);
        pools_.push_back(std::make_shared<Pool>(
            resolved_dsn,
            host.pool.min_size,
            host.pool.max_size,
            blocking_task_processor,
            statement_metrics_settings,
            prepared_statement_cache_settings
        ));
    }
}

TopologyBase::~TopologyBase() = default;

std::shared_ptr<TopologyBase> TopologyBase::Create(
    const settings::ODBCClusterSettings& settings,
    const settings::StatementMetricsSettings& statement_metrics_settings,
    const settings::PreparedStatementCacheSettings& prepared_statement_cache_settings,
    clients::dns::Resolver* resolver,
    engine::TaskProcessor& blocking_task_processor
) {
    UASSERT(!settings.pools.empty());

    if (settings.pools.size() == 1) {
        return std::make_shared<Standalone>(
            settings,
            statement_metrics_settings,
            prepared_statement_cache_settings,
            resolver,
            blocking_task_processor
        );
    }

    return std::make_shared<FixedPrimary>(
        settings,
        statement_metrics_settings,
        prepared_statement_cache_settings,
        resolver,
        blocking_task_processor
    );
}

Pool& TopologyBase::SelectPool(ClusterHostType host_type) const {
    switch (host_type) {
        case ClusterHostType::kMaster:
        case ClusterHostType::kNone:
            return GetPrimary();
        case ClusterHostType::kSlave:
            return GetSecondary();
    }

    UINVARIANT(false, "Unknown host type");
}

void TopologyBase::WriteStatistics(utils::statistics::Writer& writer) const {
    std::vector<StatementStatisticsSnapshot> statement_snapshots;
    statement_snapshots.reserve(pools_.size());
    for (const auto& pool : pools_) {
        statement_snapshots.push_back(pool->GetStatementStatistics());
    }

    for (std::size_t i = 0; i < pools_.size(); ++i) {
        const auto pool_label = std::to_string(i);
        writer.ValueWithLabels(pools_[i]->GetStatistics(), {{"odbc_pool", pool_label}});
        writer.ValueWithLabels(statement_snapshots[i], {{"odbc_pool", pool_label}});
        writer.ValueWithLabels(pools_[i]->GetPreparedStatementCacheStatistics(), {{"odbc_pool", pool_label}});
    }
}

void TopologyBase::SetPreparedStatementCacheSettings(const settings::PreparedStatementCacheSettings& settings) {
    for (const auto& pool : pools_) {
        pool->SetPreparedStatementCacheSettings(settings);
    }
}

void TopologyBase::SetStatementMetricsSettings(const settings::StatementMetricsSettings& settings) {
    for (const auto& pool : pools_) {
        pool->SetStatementMetricsSettings(settings);
    }
}

}  // namespace storages::odbc::detail::topology

USERVER_NAMESPACE_END
