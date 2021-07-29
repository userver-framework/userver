#include <userver/storages/mongo/pool.hpp>

#include <unordered_map>

#include <userver/formats/json/value_builder.hpp>
#include <userver/utils/statistics/metadata.hpp>

#include <storages/mongo/cdriver/collection_impl.hpp>
#include <storages/mongo/cdriver/pool_impl.hpp>
#include <storages/mongo/database.hpp>
#include <storages/mongo/stats_serialize.hpp>

namespace storages::mongo {
namespace {

formats::json::Value GetPoolStatistics(const impl::PoolImpl& pool_impl,
                                       stats::Verbosity verbosity) {
  formats::json::ValueBuilder builder(formats::json::Type::kObject);
  stats::PoolStatisticsToJson(pool_impl.GetStatistics(), builder, verbosity);
  builder["pool"]["current-size"] = pool_impl.SizeApprox();
  builder["pool"]["current-in-use"] = pool_impl.InUseApprox();
  builder["pool"]["max-size"] = pool_impl.MaxSize();

  utils::statistics::SolomonLabelValue(builder, "mongo_database");
  return builder.ExtractValue();
}

}  // namespace

Pool::Pool(std::string id, const std::string& uri, const PoolConfig& config)
    : impl_(std::make_shared<impl::cdriver::CDriverPoolImpl>(std::move(id), uri,
                                                             config)) {}

Pool::~Pool() = default;

bool Pool::HasCollection(const std::string& name) const {
  return impl::Database(impl_, impl_->DefaultDatabaseName())
      .HasCollection(name);
}

Collection Pool::GetCollection(std::string name) const {
  return Collection(std::make_shared<impl::cdriver::CDriverCollectionImpl>(
      impl_, impl_->DefaultDatabaseName(), std::move(name)));
}

formats::json::Value Pool::GetStatistics() const {
  return GetPoolStatistics(*impl_, stats::Verbosity::kTerse);
}

formats::json::Value Pool::GetVerboseStatistics() const {
  return GetPoolStatistics(*impl_, stats::Verbosity::kFull);
}

}  // namespace storages::mongo
