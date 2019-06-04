/*
 * database.cpp
 *
 *  Created on: Jun 4, 2019
 *      Author: ser-fedorov
 */

#include <storages/postgres/component.hpp>
#include <storages/postgres/exceptions.hpp>

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
