#include <storages/mongo/pool.hpp>

#include "collection_impl.hpp"
#include "pool_impl.hpp"

namespace storages {
namespace mongo {

Pool::Pool(const std::string& uri, engine::TaskProcessor& task_processor,
           const PoolConfig& config)
    : impl_(std::make_unique<impl::PoolImpl>(task_processor, uri, config)) {}

Pool::~Pool() = default;

Collection Pool::GetCollection(std::string collection_name) {
  return GetCollection(impl_->GetDefaultDatabaseName(),
                       std::move(collection_name));
}

Collection Pool::GetCollection(std::string database_name,
                               std::string collection_name) {
  return Collection(std::make_shared<impl::CollectionImpl>(
      impl_, std::move(database_name), std::move(collection_name)));
}

}  // namespace mongo
}  // namespace storages
