#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

ClusterPtr Database::GetCluster() const {
  return GetClusterForShard(components::Postgres::kDefaultShardNumber);
}

ClusterPtr Database::GetClusterForShard(size_t shard) const {
  if (shard >= GetShardCount()) {
    throw storages::postgres::ClusterUnavailable(
        "Shard number " + std::to_string(shard) + " is out of range");
  }

  return clusters_[shard];
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
