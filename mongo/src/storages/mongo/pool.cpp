#include <storages/mongo/pool.hpp>

#include <unordered_map>

#include <formats/json/value_builder.hpp>
#include <utils/statistics/metadata.hpp>

#include <storages/mongo/collection_impl.hpp>
#include <storages/mongo/database.hpp>
#include <storages/mongo/pool_impl.hpp>
#include <storages/mongo/stats_serialize.hpp>

namespace storages::mongo {

Pool::Pool(std::string id, const std::string& uri, const PoolConfig& config)
    : impl_(std::make_shared<impl::PoolImpl>(std::move(id), uri, config)) {}

Pool::~Pool() = default;

bool Pool::HasCollection(const std::string& name) const {
  return impl::Database(impl_, impl_->DefaultDatabaseName())
      .HasCollection(name);
}

Collection Pool::GetCollection(std::string name) const {
  return Collection(std::make_shared<impl::CollectionImpl>(
      impl_, impl_->DefaultDatabaseName(), std::move(name)));
}

formats::json::Value Pool::GetStatistics() const {
  formats::json::ValueBuilder builder(formats::json::Type::kObject);
  stats::PoolStatisticsToJson(impl_->GetStatistics(), builder);
  builder["pool"]["current-size"] = impl_->SizeApprox();
  builder["pool"]["current-in-use"] = impl_->InUseApprox();
  builder["pool"]["max-size"] = impl_->MaxSize();

  utils::statistics::SolomonLabelValue(builder, "mongo_database");
  return builder.ExtractValue();
}

}  // namespace storages::mongo
