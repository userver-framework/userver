#include <storages/mongo/pool.hpp>

#include <storages/mongo/collection_impl.hpp>
#include <storages/mongo/database.hpp>
#include <storages/mongo/pool_impl.hpp>

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

}  // namespace storages::mongo
