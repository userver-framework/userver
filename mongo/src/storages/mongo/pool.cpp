#include <userver/storages/mongo/pool.hpp>

#include <storages/mongo/cdriver/collection_impl.hpp>
#include <storages/mongo/cdriver/pool_impl.hpp>
#include <storages/mongo/database.hpp>
#include <storages/mongo/stats_serialize.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {
namespace {

const PoolConfig& ValidateConfig(const PoolConfig& config,
                                 const std::string& id) {
  config.Validate(id);
  return config;
}

}  // namespace

Pool::Pool(std::string id, const std::string& uri,
           const PoolConfig& pool_config, clients::dns::Resolver* dns_resolver,
           dynamic_config::Source config_source)
    : impl_(std::make_shared<impl::cdriver::CDriverPoolImpl>(
          std::move(id), uri, ValidateConfig(pool_config, id), dns_resolver,
          config_source)) {}

void Pool::Start() { impl_->Start(); }

void Pool::Stop() { impl_->Stop(); }

Pool::~Pool() = default;

void Pool::DropDatabase() {
  impl::Database(impl_, impl_->DefaultDatabaseName()).DropDatabase();
}

void Pool::Ping() { impl_->Ping(); }

bool Pool::HasCollection(const std::string& name) const {
  return impl::Database(impl_, impl_->DefaultDatabaseName())
      .HasCollection(name);
}

Collection Pool::GetCollection(std::string name) const {
  return Collection(std::make_shared<impl::cdriver::CDriverCollectionImpl>(
      impl_, impl_->DefaultDatabaseName(), std::move(name)));
}

std::vector<std::string> Pool::ListCollectionNames() const {
  return impl::Database(impl_, impl_->DefaultDatabaseName())
      .ListCollectionNames();
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
}

void Pool::SetPoolSettings(const PoolSettings& pool_settings) {
  impl_->SetPoolSettings(pool_settings);
}

}  // namespace storages::mongo

USERVER_NAMESPACE_END
