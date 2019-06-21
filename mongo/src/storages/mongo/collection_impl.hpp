#pragma once

#include <memory>
#include <tuple>

#include <storages/mongo/pool_impl.hpp>
#include <storages/mongo/stats.hpp>
#include <tracing/span.hpp>

namespace storages::mongo::impl {

class CollectionImpl {
 public:
  CollectionImpl(PoolImplPtr pool_impl, std::string database_name,
                 std::string collection_name);

  const std::string& GetDatabaseName() const;
  const std::string& GetCollectionName() const;

  BoundClientPtr GetClient();
  std::tuple<BoundClientPtr, CollectionPtr> GetNativeCollection();

  tracing::Span MakeSpan(const std::string& name) const;
  stats::CollectionStatistics& GetCollectionStatistics();

 private:
  PoolImplPtr pool_impl_;
  std::string database_name_;
  std::string collection_name_;
  std::shared_ptr<stats::CollectionStatistics> statistics_;
};

}  // namespace storages::mongo::impl
