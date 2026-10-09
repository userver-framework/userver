#include <userver/storages/mongo/pool.hpp>

#include <utility>

#include <storages/mongo/cdriver/collection_impl.hpp>
#include <storages/mongo/cdriver/pool_impl.hpp>
#include <storages/mongo/features.hpp>
#ifdef USERVER_FEATURE_MONGO_EXPERIMENTAL
#include <storages/mongo/cdriver_experimental/pool_impl.hpp>
#endif
#include <storages/mongo/database.hpp>
#include <storages/mongo/stats_serialize.hpp>
#include <storages/mongo/transaction_impl.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/resource_scopes.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {
Pool::Pool(
    std::string id,
    const std::string& uri,
    const PoolConfig& pool_config,
    clients::dns::Resolver* dns_resolver,
    dynamic_config::Source config_source
)
    : driver_impl_(pool_config.driver_impl)
{
    pool_config.Validate(id);
    LOG_INFO()
        << "MongoDB pool '" << id << "' uses "
        << (driver_impl_ == PoolConfig::DriverImpl::kMongoCDriverExperimental
                ? "mongo-c-driver-experimental"
                : "mongo-c-driver");
#ifdef USERVER_FEATURE_MONGO_EXPERIMENTAL
    if (driver_impl_ == PoolConfig::DriverImpl::kMongoCDriverExperimental) {
        impl_ = utils::MakeWithResourceScopes<
            impl::cdriver_experimental::CDriverPoolImpl>(std::move(id), uri, pool_config, dns_resolver, config_source);
    } else {
#endif
        impl_ = utils::MakeWithResourceScopes<
            impl::cdriver::CDriverPoolImpl>(std::move(id), uri, pool_config, dns_resolver, config_source);
#ifdef USERVER_FEATURE_MONGO_EXPERIMENTAL
    }
#endif
}

Pool::Pool(Pool&&) noexcept = default;

Pool& Pool::operator=(Pool&&) noexcept = default;

Pool::~Pool() = default;

void Pool::DropDatabase() { impl::Database(impl_).DropDatabase(); }

void Pool::Ping() { impl_->Ping(); }

bool Pool::HasCollection(utils::zstring_view name) const { return impl::Database(impl_).HasCollection(name); }

Collection Pool::GetCollection(std::string name) const {
    return Collection(std::make_shared<impl::cdriver::CDriverCollectionImpl>(impl_, std::move(name)));
}

std::vector<std::string> Pool::ListCollectionNames() const { return impl::Database(impl_).ListCollectionNames(); }

Transaction Pool::BeginTransaction() const {
    auto transaction_impl = std::make_unique<impl::TransactionImpl>(impl_);
    return Transaction{std::move(transaction_impl)};
}

Cursor Pool::Execute(const operations::Aggregate& aggregate_op) {
    return impl::Database(impl_).Aggregate(aggregate_op);
}

void DumpMetric(utils::statistics::Writer& writer, const Pool& pool) {
    const auto verbosity = pool.impl_->GetStatsVerbosity();
    if (verbosity == StatsVerbosity::kNone) {
        return;
    }

    stats::DumpMetric(writer, pool.impl_->GetStatistics(), verbosity);
    if (auto pool_metrics = writer["pool"]) {
        pool_metrics["current-size"] = pool.impl_->SizeApprox();
        pool_metrics["current-in-use"] = pool.impl_->InUseApprox();
        pool_metrics["max-size"] = pool.impl_->MaxSize();
    }
    if (auto apm_metrics = writer["pool"]["apm"]) {
        const auto& apm = pool.impl_->GetApmStats();
        apm_metrics["topology-changed"] = apm.topology.changed;
        apm_metrics["heartbeats-start"] = apm.heartbeats.start;
        apm_metrics["heartbeats-success"] = apm.heartbeats.success;
        apm_metrics["heartbeats-failed"] = apm.heartbeats.failed;
    }
}

void Pool::SetPoolSettings(const PoolSettings& pool_settings) { impl_->SetPoolSettings(pool_settings); }

void Pool::SetConnectionString(const std::string& connection_string) { impl_->SetConnectionString(connection_string); }

}  // namespace storages::mongo

USERVER_NAMESPACE_END
